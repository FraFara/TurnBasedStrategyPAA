// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Brawler.h"
#include "Sniper.h"
#include "TBS_HumanPlayer.h"
#include "TBS_NaiveAI.h"
#include "TBS_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "TBS_GameInstance.h"
#include "TBS_PlayerInterface.h"
#include "EngineUtils.h"
#include "Components/Widget.h"

ATBS_GameMode::ATBS_GameMode()
{
    // Default values
    NumberOfPlayers = 2;
    UnitsPerPlayer = 2;
    CurrentPlayer = 0;
    GridSize = 25;
    CurrentPhase = EGamePhase::NONE;
    UnitsPlaced = 0;
    bIsGameOver = false;
    ObstaclePercentage = 10.0f;     // Random default obstacle percentage

    // Initialize unit placement tracking
    BrawlerPlaced.Init(false, NumberOfPlayers);
    SniperPlaced.Init(false, NumberOfPlayers);

    // Set default player controller class
    PlayerControllerClass = ATBS_PlayerController::StaticClass();

    // Set default pawn class
    DefaultPawnClass = ATBS_HumanPlayer::StaticClass();

    // Load the Brawler and Sniper classes (add this)
    static ConstructorHelpers::FClassFinder<ABrawler> BrawlerBP(TEXT("/Game/Blueprints/BP_Brawler"));
    if (BrawlerBP.Class)
    {
        BrawlerClass = BrawlerBP.Class;
    }

    static ConstructorHelpers::FClassFinder<ASniper> SniperBP(TEXT("/Game/Blueprints/BP_Sniper"));
    if (SniperBP.Class)
    {
        SniperClass = SniperBP.Class;
    }
}

void ATBS_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Span Grid
    if (GridClass != nullptr)
    {
        GameGrid = GetWorld()->SpawnActor<AGrid>(GridClass);
        if (GameGrid)
        {
            GameGrid->Size = GridSize;
            GameGrid->GenerateGrid(); // Ensure grid is generated before other operations
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn grid"));
            return;
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GridClass is null"));
        return;
    }


    // Find the human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Player Controller found"));
        return;
    }

    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
    if (!HumanPlayer)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Spawning HumanPlayer"));

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = PlayerController;
        HumanPlayer = GetWorld()->SpawnActor<ATBS_HumanPlayer>(ATBS_HumanPlayer::StaticClass(), SpawnParams);

        if (HumanPlayer)
        {
            PlayerController->Possess(HumanPlayer);
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("HumanPlayer spawned successfully"));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn HumanPlayer"));
            return;
        }
    }

    // Ensure Players array is properly populated
    Players.Empty();
    Players.Add(HumanPlayer);

    // Spawn AI player ONCE
    auto* AI = GetWorld()->SpawnActor<ATBS_NaiveAI>(FVector(), FRotator());
    if (AI)
    {
        Players.Add(AI);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("AI Player added successfully"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn AI Player"));
    }
    // Find all players (Human and AI) -> added in case of more players
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTBS_PlayerInterface::StaticClass(), FoundPlayers);

    // Set initial values for units per player
    UnitsRemaining.SetNum(NumberOfPlayers);
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        UnitsRemaining[i] = UnitsPerPlayer;
    }


    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("BeginPlay - Setting up players"));

    float CameraPosX = ((GameGrid->TileSize * GridSize) + ((GridSize - 1) * GameGrid->TileSize * GameGrid->CellPadding)) * 0.5f;
    float Zposition = 3000.0f;
    FVector CameraPos(CameraPosX, CameraPosX, Zposition);
    HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

    // Add debug messages for game start
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Game Starting - Coin Toss"));

    // Place some obstacles 
    SpawnObstacles();

    // Start the game with a coin toss
    int32 StartingPlayer = SimulateCoinToss();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Coin Toss Result: Player %d starts"), StartingPlayer));

    StartPlacementPhase(StartingPlayer);
}

void ATBS_GameMode::ShowUnitSelectionUI(bool bContextAware)
{
    // Ensure we're in the setup phase and the current player is correct
    if (CurrentPhase != EGamePhase::SETUP || CurrentPlayer != 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Cannot show unit selection UI outside of setup phase or when it's not the human player's turn"));
        return;
    }

    // Get the human player to check the current placement tile
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    ATBS_HumanPlayer* HumanPlayer = PlayerController ?
        Cast<ATBS_HumanPlayer>(PlayerController->GetPawn()) : nullptr;

    // Validate human player and placement tile
    if (!HumanPlayer || !HumanPlayer->GetCurrentPlacementTile())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("No tile selected for placement or invalid human player"));
        return;
    }

    // Clear existing widget
    if (UnitSelectionWidget && UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Create a new widget instance
    if (UnitSelectionWidgetClass)
    {
        // Create the widget with explicit player controller
        UnitSelectionWidget = CreateWidget<UUserWidget>(PlayerController, UnitSelectionWidgetClass);

        if (UnitSelectionWidget)
        {
            // Add it to viewport with high Z-Order to ensure it's on top
            UnitSelectionWidget->AddToViewport(100);

            // Enable input for the player controller
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(UnitSelectionWidget->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = true;

            // Find buttons
            UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));
            UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));

            // Validate buttons exist
            if (!BrawlerButton || !SniperButton)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                    TEXT("Failed to find unit selection buttons"));
                UnitSelectionWidget->RemoveFromParent();
                UnitSelectionWidget = nullptr;
                return;
            }

            // Hide buttons for already placed units
            if (bContextAware)
            {
                if (BrawlerPlaced[CurrentPlayer])
                {
                    Cast<UWidget>(BrawlerButton)->SetVisibility(ESlateVisibility::Hidden);
                }

                if (SniperPlaced[CurrentPlayer])
                {
                    Cast<UWidget>(SniperButton)->SetVisibility(ESlateVisibility::Hidden);
                }
            }

            // If both are placed, hide the widget
            if (BrawlerPlaced[CurrentPlayer] && SniperPlaced[CurrentPlayer])
            {
                UnitSelectionWidget->RemoveFromParent();
                UnitSelectionWidget = nullptr;

                // Restore game input
                PlayerController->SetInputMode(FInputModeGameOnly());
            }

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                TEXT("Unit selection UI shown successfully"));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Failed to create UnitSelectionWidget"));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("UnitSelectionWidgetClass is not set"));
    }
}

void ATBS_GameMode::HideUnitSelectionUI()
{
    if (UnitSelectionWidget && UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;

        // Restore game input mode
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
        }
    }
}

void ATBS_GameMode::OnUnitTypeSelected(EUnitType SelectedType)
{
    // Debug logging
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("OnUnitTypeSelected called with type: %s"),
            *UEnum::GetValueAsString(SelectedType)));

    // Hide the selection UI
    HideUnitSelectionUI();

    // Get the current human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Player Controller found"));
        return;
    }

    // Cast to human player
    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
    if (!HumanPlayer)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to cast to HumanPlayer"));
        return;
    }

    // Retrieve the current placement tile from the human player
    ATile* PlacementTile = HumanPlayer->GetCurrentPlacementTile();
    if (!PlacementTile)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No tile selected for placement"));
        return;
    }

    // Get grid position of the tile
    FVector2D GridPos = PlacementTile->GetGridPosition();

    // Attempt to place the unit
    bool Success = PlaceUnit(
        SelectedType,
        GridPos.X,
        GridPos.Y,
        CurrentPlayer
    );

    if (Success)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("Successfully placed %s at (%f, %f)"),
                *UEnum::GetValueAsString(SelectedType), GridPos.X, GridPos.Y));

        // Clear the placement tile in the human player
        HumanPlayer->ClearCurrentPlacementTile();
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Failed to place unit"));
    }
}

// SimulateCoinToss to show the result
int32 ATBS_GameMode::SimulateCoinToss()
{
    // Existing code
    int32 StartingPlayer = FMath::RandRange(0, 1);

    // Get game instance and set the starting player message
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->SetStartingPlayerMessage(StartingPlayer);
    }

    // Show the coin toss result
    ShowCoinTossResult();

    return StartingPlayer;
}

void ATBS_GameMode::ShowCoinTossResult()
{
    if (!CoinTossWidgetClass)
        return;

    // Get the current turn message from the game instance
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (!GameInstance)
        return;

    FString CoinTossMessage = GameInstance->GetTurnMessage();

    // Display in debug log
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, CoinTossMessage);

    // Create widget to show the result
    CoinTossWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), CoinTossWidgetClass);
    if (CoinTossWidget)
    {
        CoinTossWidget->AddToViewport();

        // Try to find and update the result text
        UTextBlock* ResultText = Cast<UTextBlock>(CoinTossWidget->GetWidgetFromName(TEXT("ResultText")));
        if (ResultText)
        {
            ResultText->SetText(FText::FromString(CoinTossMessage));
        }

        // Auto-remove after a few seconds
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [this]()
            {
                if (CoinTossWidget && CoinTossWidget->IsInViewport())
                {
                    CoinTossWidget->RemoveFromParent();
                    CoinTossWidget = nullptr;
                }
            }, 3.0f, false);
    }
}

void ATBS_GameMode::DebugForceGameplay()
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FORCING GAMEPLAY PHASE START"));
    UnitsPlaced = NumberOfPlayers * UnitsPerPlayer;
    StartGameplayPhase();
}

void ATBS_GameMode::StartPlacementPhase(int32 StartingPlayer)
{
    CurrentPlayer = StartingPlayer;
    CurrentPhase = EGamePhase::SETUP;

    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, TEXT("StartPlacementPhase called")); // To check where is the problem


    // Notify the player it's their turn to place
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];

        // Get the interface pointer
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);

        // Call the OnPlacement function
        ITBS_PlayerInterface::Execute_OnPlacement(PlayerActor);

        // Show unit selection UI if it's the human player's turn (player 0)
        if (CurrentPlayer == 0)
        {
            // Add a debug message to verify this is called
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Showing Unit Selection UI"));

            ShowUnitSelectionUI();
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
                FString::Printf(TEXT("Not showing UI for player %d"), CurrentPlayer));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Player index not valid"));
    }
}

void ATBS_GameMode::StartGameplayPhase()
{
    // Explicitly set the phase to GAMEPLAY
    CurrentPhase = EGamePhase::GAMEPLAY;

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        TEXT("GAMEPLAY PHASE STARTED"));

    // Reset any placement-related flags
    UnitsPlaced = NumberOfPlayers * UnitsPerPlayer;

    // IMPORTANT: Reset all placement flags to avoid any confusion
    for (int32 i = 0; i < BrawlerPlaced.Num(); i++)
    {
        BrawlerPlaced[i] = true;
        SniperPlaced[i] = true;
    }

    // Clear any UI widgets that might be causing interference
    if (UnitSelectionWidget && UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // THIS IS IMPORTANT: Reset player actions state
    for (AActor* PlayerActor : Players)
    {
        if (ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerActor))
        {
            HumanPlayer->ResetActionState();
        }
        else if (ATBS_NaiveAI* AIPlayer = Cast<ATBS_NaiveAI>(PlayerActor))
        {
            AIPlayer->ResetActionState();
        }
    }

    // Reset all units for new turn
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(Actor);
        if (Unit)
        {
            Unit->ResetTurn();
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                FString::Printf(TEXT("Reset unit: %s (Owner: %d)"),
                    *Unit->GetUnitName(), Unit->GetOwnerID()));
        }
    }

    // Ensure proper input mode for the player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        ATBS_PlayerController* TBS_PC = Cast<ATBS_PlayerController>(PC);
        if (TBS_PC)
        {
            TBS_PC->SetGameInputMode();
        }
    }

    // Set turn states explicitly for all players
    for (int32 i = 0; i < Players.Num(); i++)
    {
        if (Players.IsValidIndex(i))
        {
            // Only the current player gets turn set to true
            bool bIsCurrentPlayer = (i == CurrentPlayer);
            ITBS_PlayerInterface::Execute_SetTurnState(Players[i], bIsCurrentPlayer);

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
                FString::Printf(TEXT("Player %d turn state set to: %s"),
                    i, bIsCurrentPlayer ? TEXT("TRUE") : TEXT("FALSE")));
        }
    }

    // Ensure the current player is set correctly
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
        FString::Printf(TEXT("First player in GAMEPLAY phase: %d"), CurrentPlayer));

    // Notify the player it's their turn (with delay to ensure everything is set up)
    FTimerHandle TurnStartTimerHandle;
    GetWorldTimerManager().SetTimer(TurnStartTimerHandle, [this]()
        {
            if (Players.IsValidIndex(CurrentPlayer))
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                    FString::Printf(TEXT("Starting turn for Player %d"), CurrentPlayer));

                ITBS_PlayerInterface::Execute_OnTurn(Players[CurrentPlayer]);
            }
        }, 0.5f, false);
}

//
//        // Reset all units for new turn
//        TArray<AActor*> AllUnits;
//        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);
//
//        for (AActor* Actor : AllUnits)
//        {
//            AUnit* Unit = Cast<AUnit>(Actor);
//
//            Unit->ResetTurn();
//
//        }
//
//        // Notify the player it's their turn
//        AActor* PlayerActor = Players[CurrentPlayer];
//
//        // Get the interface pointer
//        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);
//
//        // Call the OnTurn function
//        PlayerInterface->OnTurn();
//    }
//}

void ATBS_GameMode::EndTurn()
{
    // Let the current player know their turn is ending
    if (Players.IsValidIndex(CurrentPlayer))
    {
        ITBS_PlayerInterface::Execute_SetTurnState(Players[CurrentPlayer], false);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("EndTurn - Ending turn for player %d"), CurrentPlayer));
    }

    // Skip to next player
    CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;

    // Check if the game is over
    if (bIsGameOver)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Game is over, not continuing"));
        return;
    }

    // Reset all units for the new player's turn
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(Actor);
        if (Unit && Unit->GetOwnerID() == CurrentPlayer)
        {
            Unit->ResetTurn();
        }
    }

    // Make sure to notify the new current player
    if (Players.IsValidIndex(CurrentPlayer))
    {
        // Set new current player's turn state to true
        ITBS_PlayerInterface::Execute_SetTurnState(Players[CurrentPlayer], true);

        // Call the OnTurn function
        ITBS_PlayerInterface::Execute_OnTurn(Players[CurrentPlayer]);

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("EndTurn - Starting turn for player %d"), CurrentPlayer));
    }
}

bool ATBS_GameMode::PlaceUnit(EUnitType Type, int32 GridX, int32 GridY, int32 PlayerIndex)
{
    // Early validation checks
    if (!GameGrid || CurrentPhase != EGamePhase::SETUP || PlayerIndex != CurrentPlayer)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Cannot place unit: Invalid game state. Grid: %s, Phase: %d, PlayerIndex: %d, CurrentPlayer: %d"),
                GameGrid ? TEXT("Valid") : TEXT("Null"),
                (int32)CurrentPhase, PlayerIndex, CurrentPlayer));
        return false;
    }

    // Debug the current placement state
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("PlaceUnit - Player %d placing %s, BrawlerPlaced: %d, SniperPlaced: %d"),
            PlayerIndex, (Type == EUnitType::BRAWLER) ? TEXT("Brawler") : TEXT("Sniper"),
            BrawlerPlaced[PlayerIndex], SniperPlaced[PlayerIndex]));

    // Check if player already placed this unit type
    if ((Type == EUnitType::BRAWLER && BrawlerPlaced[PlayerIndex]) ||
        (Type == EUnitType::SNIPER && SniperPlaced[PlayerIndex]))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("You've already placed a %s unit"),
                (Type == EUnitType::BRAWLER) ? TEXT("Brawler") : TEXT("Sniper")));
        return false;
    }

    // Validate tile
    FVector2D Position(GridX, GridY);
    if (!GameGrid->TileMap.Contains(Position))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Invalid tile for unit placement: (%d, %d)"), GridX, GridY));
        return false;
    }

    ATile* Tile = GameGrid->TileMap[Position];
    if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Selected tile is not empty. Tile: %s, Status: %d"),
                Tile ? TEXT("Valid") : TEXT("Null"), Tile ? (int32)Tile->GetTileStatus() : -1));
        return false;
    }

    // Spawn unit - ensure both classes are set
    if (!BrawlerClass || !SniperClass)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Unit class not set. Brawler: %s, Sniper: %s"),
                BrawlerClass ? TEXT("Valid") : TEXT("Null"),
                SniperClass ? TEXT("Valid") : TEXT("Null")));
        return false;
    }

    // Spawn unit
    AUnit* NewUnit = nullptr;
    FVector SpawnLocation = Tile->GetActorLocation() + FVector(0, 0, 10.0f);

    NewUnit = GetWorld()->SpawnActor<AUnit>(
        (Type == EUnitType::BRAWLER) ? BrawlerClass : SniperClass,
        SpawnLocation,
        FRotator::ZeroRotator
    );

    if (!NewUnit)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Failed to spawn unit"));
        return false;
    }

    // Initialize unit
    NewUnit->SetOwnerID(PlayerIndex);
    NewUnit->InitializePosition(Tile);

    // Debug logging
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("Unit placed - Type: %s, Owner: %d, Position: (%d,%d)"),
            *NewUnit->GetUnitName(), NewUnit->GetOwnerID(), GridX, GridY));

    // Get game instance to record move history
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Format the unit type string
        FString UnitTypeString = (Type == EUnitType::BRAWLER) ? "Brawler" : "Sniper";

        // Record the placement move
        GameInstance->AddMoveToHistory(
            PlayerIndex,
            UnitTypeString,
            "Place",
            FVector2D::ZeroVector,
            FVector2D(GridX, GridY),
            0
        );

        // Additional debug logging
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("Recorded placement of %s at (%d, %d)"), *UnitTypeString, GridX, GridY));
    }

    // Mark unit type as placed
    if (Type == EUnitType::BRAWLER)
        BrawlerPlaced[PlayerIndex] = true;
    else if (Type == EUnitType::SNIPER)
        SniperPlaced[PlayerIndex] = true;

    // Increment units placed
    UnitsPlaced++;

    // Debug the units placed count
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("Units placed: %d of %d"), UnitsPlaced, NumberOfPlayers * UnitsPerPlayer));

    // Check if all units are placed
    if (UnitsPlaced >= NumberOfPlayers * UnitsPerPlayer)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("All units placed, starting gameplay phase"));

        // IMPORTANT: Add a delay before starting gameplay phase
        FTimerHandle GameplayTimerHandle;
        GetWorldTimerManager().SetTimer(GameplayTimerHandle, [this]()
            {
                StartGameplayPhase();
            }, 1.0f, false);

        return true;
    }

    // Switch to next player for placement
    CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("Next player for placement: %d"), CurrentPlayer));

    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);
        if (PlayerInterface)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
                FString::Printf(TEXT("Calling OnPlacement for player %d"), CurrentPlayer));
            ITBS_PlayerInterface::Execute_OnPlacement(PlayerActor);
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("PlayerInterface cast failed"));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Invalid player index: %d, Players.Num(): %d"),
                CurrentPlayer, Players.Num()));
    }

    return true;
}

bool ATBS_GameMode::CheckGameOver()
{
    bool bGameOver = false;
    int32 WinningPlayer = -1;

    // Check each player's remaining units
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        TArray<AActor*> PlayerUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), PlayerUnits);

        int32 RemainingUnits = 0;
        for (AActor* Actor : PlayerUnits)
        {
            AUnit* Unit = Cast<AUnit>(Actor);
            if (Unit && Unit->GetOwnerID() == i)
            {
                RemainingUnits++;
            }
        }

        UnitsRemaining[i] = RemainingUnits;

        // If a player has no units, the other player wins
        if (RemainingUnits == 0)
        {
            bGameOver = true;
            WinningPlayer = (i + 1) % NumberOfPlayers; // Other player wins
        }
    }

    if (bGameOver && !bIsGameOver)
    {
        bIsGameOver = true;
        PlayerWon(WinningPlayer);
    }

    return bGameOver;
}

void ATBS_GameMode::NotifyUnitDestroyed(int32 PlayerIndex)
{
    // Decrement the player's unit count
    if (UnitsRemaining.IsValidIndex(PlayerIndex))
    {
        UnitsRemaining[PlayerIndex]--;

        // Check if the game is over
        CheckGameOver();
    }
}

AActor* ATBS_GameMode::GetCurrentPlayer()
{
    if (Players.IsValidIndex(CurrentPlayer))
    {
        return Players[CurrentPlayer];
    }

    return nullptr;
}

void ATBS_GameMode::PlayerWon(int32 PlayerIndex)
{
    // Notify all players about the winner
    for (int32 i = 0; i < Players.Num(); i++)
    {
        if (Players.IsValidIndex(i))
        {
            AActor* PlayerActor = Players[i];
            if (PlayerActor)
            {
                // Get the interface pointer
                ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);
                if (PlayerInterface)
                {
                    if (i == PlayerIndex)
                    {
                        // Call the OnWin function
                        ITBS_PlayerInterface::Execute_OnWin(PlayerActor);
                    }
                    else
                    {
                        // Call the OnLose function
                        ITBS_PlayerInterface::Execute_OnLose(PlayerActor);
                    }
                }
            }
        }
    }
}

void ATBS_GameMode::SpawnObstacles()
{
    // Validate grid existence
    if (!GameGrid)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot spawn obstacles: Grid is null"));
        return;
    }

    if (ObstaclePercentage <= 0.0f)
        return;

    // Calculate number of obstacles to place
    int32 TotalCells = GridSize * GridSize;
    int32 ObstaclesToPlace = FMath::RoundToInt((ObstaclePercentage / 100.0f) * TotalCells);

    // Validate grid map
    if (GameGrid->TileMap.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot spawn obstacles: TileMap is empty"));
        return;
    }

    // Simple random placement for now
    int32 PlacedObstacles = 0;
    int32 MaxAttempts = ObstaclesToPlace * 10; // Prevent infinite loop
    int32 Attempts = 0;

    while (PlacedObstacles < ObstaclesToPlace && Attempts < MaxAttempts)
    {
        int32 RandomX = FMath::RandRange(0, GridSize - 1);
        int32 RandomY = FMath::RandRange(0, GridSize - 1);

        // Safely check if the tile exists
        FVector2D Position(RandomX, RandomY);
        if (GameGrid->TileMap.Contains(Position))
        {
            ATile* Tile = GameGrid->TileMap[Position];

            if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY)
            {
                // Mark as obstacles
                Tile->SetTileStatus(-2, ETileStatus::OCCUPIED); // -2 for obstacles

                PlacedObstacles++;
            }
        }

        Attempts++;
    }

    // Log obstacle placement
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("Placed %d obstacles out of %d attempts"), PlacedObstacles, Attempts));
}


void ATBS_GameMode::RecordMove(int32 PlayerIndex, FString UnitType, FString ActionType,
    FVector2D FromPosition, FVector2D ToPosition, int32 Damage)
{
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

    GameInstance->AddMoveToHistory(PlayerIndex, UnitType, ActionType, FromPosition, ToPosition, Damage);

}