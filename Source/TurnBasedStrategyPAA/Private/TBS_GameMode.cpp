// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Brawler.h"
#include "Sniper.h"
#include "TBS_HumanPlayer.h"
#include "TBS_NaiveAI.h"
#include "TBS_SmartAI.h"
#include "AISelectionWidget.h"
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

    NaiveAIClass = ATBS_NaiveAI::StaticClass();
    SmartAIClass = ATBS_SmartAI::StaticClass();
    AISelectionWidgetClass = UAISelectionWidget::StaticClass(); 

}

void ATBS_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Initialize the Grid first
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
            return;
        }
    }
    else
    {
        return;
    }

    // Find/Initialize the human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        return;
    }

    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
    if (!HumanPlayer)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = PlayerController;
        HumanPlayer = GetWorld()->SpawnActor<ATBS_HumanPlayer>(ATBS_HumanPlayer::StaticClass(), SpawnParams);

        if (HumanPlayer)
        {
            PlayerController->Possess(HumanPlayer);
        }
        else
        {
            return;
        }
    }

    // Set up camera position for the human player
    float CameraPosX = ((GameGrid->TileSize * GridSize) + ((GridSize - 1) * GameGrid->TileSize * GameGrid->CellPadding)) * 0.5f;
    float Zposition = 3000.0f;
    FVector CameraPos(CameraPosX, CameraPosX, Zposition);
    HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

    // Temporarily add HumanPlayer to Players array
    Players.Empty();
    Players.Add(HumanPlayer);

    // Set initial values for units per player
    UnitsRemaining.SetNum(NumberOfPlayers);
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        UnitsRemaining[i] = UnitsPerPlayer;
    }

    // Start with AI selection phase
    CurrentPhase = EGamePhase::AI_SELECTION;

    // Initialize default value
    bUseSmartAI = false;

    // Add a slight delay before showing the AI selection UI
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ATBS_GameMode::ShowAISelectionUI, 0.5f, false);
}

void ATBS_GameMode::ShowUnitSelectionUI(bool bContextAware)
{
    // Ensure we're in the setup phase and the current player is correct
    if (CurrentPhase != EGamePhase::SETUP || CurrentPlayer != 0)
    {
        return;
    }

    // Get the human player to check the current placement tile
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    ATBS_HumanPlayer* HumanPlayer = PlayerController ?
        Cast<ATBS_HumanPlayer>(PlayerController->GetPawn()) : nullptr;

    // Validate human player and placement tile
    if (!HumanPlayer || !HumanPlayer->GetCurrentPlacementTile())
    {
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
        }
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
    // Hide the selection UI
    HideUnitSelectionUI();

    // Get the current human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        return;
    }

    // Cast to human player
    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
    if (!HumanPlayer)
    {
        return;
    }

    // Retrieve the current placement tile from the human player
    ATile* PlacementTile = HumanPlayer->GetCurrentPlacementTile();
    if (!PlacementTile)
    {
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
        // Clear the placement tile in the human player
        HumanPlayer->ClearCurrentPlacementTile();
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Failed to place unit"));
    }
}

void ATBS_GameMode::ShowAISelectionUI()
{
    // Clear any existing widgets
    HideUnitSelectionUI();
    if (CoinTossWidget && CoinTossWidget->IsInViewport())
    {
        CoinTossWidget->RemoveFromParent();
        CoinTossWidget = nullptr;
    }
    if (AISelectionWidget && AISelectionWidget->IsInViewport())
    {
        AISelectionWidget->RemoveFromParent();
        AISelectionWidget = nullptr;
    }

    // Create a new AI selection widget
    if (AISelectionWidgetClass)
    {
        // Get the player controller
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (!PlayerController)
        {
            return;
        }

        // Create the widget
        AISelectionWidget = CreateWidget<UUserWidget>(PlayerController, AISelectionWidgetClass);

        if (AISelectionWidget)
        {
            // Add it to viewport
            AISelectionWidget->AddToViewport(100);

            // Enable input for the player controller
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(AISelectionWidget->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = true;

            // Find buttons if they exist
            UButton* EasyButton = Cast<UButton>(AISelectionWidget->GetWidgetFromName(TEXT("EasyButton")));
            UButton* HardButton = Cast<UButton>(AISelectionWidget->GetWidgetFromName(TEXT("HardButton")));

            // Set up callbacks if buttons exist
            if (EasyButton)
            {
                EasyButton->OnClicked.AddDynamic(this, &ATBS_GameMode::OnEasyAISelected);
            }

            if (HardButton)
            {
                HardButton->OnClicked.AddDynamic(this, &ATBS_GameMode::OnHardAISelected);
            }
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create AI Selection Widget"));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AISelectionWidgetClass is not set"));
    }
}

void ATBS_GameMode::HideAISelectionUI()
{
    if (AISelectionWidget && AISelectionWidget->IsInViewport())
    {
        AISelectionWidget->RemoveFromParent();
        AISelectionWidget = nullptr;

        // Restore game input mode
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
        }
    }
}

void ATBS_GameMode::OnEasyAISelected()
{
    bUseSmartAI = false;

    // Hide the selection UI
    HideAISelectionUI();

    // Update GameInstance with a message
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

    // Proceed with game initialization
    InitializeGame();
}

void ATBS_GameMode::OnHardAISelected()
{
    bUseSmartAI = true;

    // Hide the selection UI
    HideAISelectionUI();

    // Update GameInstance with a message
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->SetTurnMessage(TEXT("Hard AI (Smart) selected - Starting game..."));
    }

    // Proceed with game initialization
    InitializeGame();
}

void ATBS_GameMode::OnAIDifficultySelected(bool bIsHardAI)
{
    // Set the AI difficulty based on the parameter
    this->bUseSmartAI = bIsHardAI;

    // Hide the selection UI
    HideAISelectionUI();

    // Update GameInstance with a message
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->SetTurnMessage(FString::Printf(TEXT("%s AI selected - Starting game..."),
            bIsHardAI ? TEXT("Hard (Smart)") : TEXT("Easy (Random)")));
    }

    // Proceed with game initialization
    InitializeGame();
}

void ATBS_GameMode::InitializeGame()
{
    // Spawn Grid if not already spawned
    if (!GameGrid && GridClass != nullptr)
    {
        GameGrid = GetWorld()->SpawnActor<AGrid>(GridClass);
        if (GameGrid)
        {
            GameGrid->Size = GridSize;
            GameGrid->GenerateGrid();
        }
        else
        {
            return;
        }
    }

    // Use a timer to delay obstacle spawning and subsequent game initialization
    FTimerHandle ObstacleSpawnTimerHandle;
    GetWorldTimerManager().SetTimer(ObstacleSpawnTimerHandle, [this]()
        {
            // Spawn obstacles
            SpawnObstaclesWithConnectivity();

            // Find the human player
            APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
            if (!PlayerController)
            {
                return;
            }

            ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
            if (!HumanPlayer)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = PlayerController;
                HumanPlayer = GetWorld()->SpawnActor<ATBS_HumanPlayer>(ATBS_HumanPlayer::StaticClass(), SpawnParams);
                if (HumanPlayer)
                {
                    PlayerController->Possess(HumanPlayer);
                }
                else
                {
                    return;
                }
            }

            // Ensure Players array is properly populated
            Players.Empty();
            Players.Add(HumanPlayer);

            // Spawn the selected AI player
            AActor* AI = nullptr;
            if (bUseSmartAI)
            {
                if (SmartAIClass)
                {
                    AI = GetWorld()->SpawnActor<AActor>(SmartAIClass, FVector(), FRotator());
                }
                else
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SmartAIClass not set"));
                }
            }
            else
            {
                if (NaiveAIClass)
                {
                    AI = GetWorld()->SpawnActor<AActor>(NaiveAIClass, FVector(), FRotator());
                }
                else
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("NaiveAIClass not set"));
                }
            }

            if (AI)
            {
                Players.Add(AI);
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn AI Player"));
            }

            // Set initial values for units per player
            UnitsRemaining.SetNum(NumberOfPlayers);
            for (int32 i = 0; i < NumberOfPlayers; i++)
            {
                UnitsRemaining[i] = UnitsPerPlayer;
            }

            // Start the game with a coin toss
            int32 StartingPlayer = SimulateCoinToss();
            StartPlacementPhase(StartingPlayer);
        }, 0.1f, false); // Short delay to allow obstacle spawning to complete
}

// SimulateCoinToss to show the result
int32 ATBS_GameMode::SimulateCoinToss()
{

    int32 StartingPlayer = FMath::RandRange(0, 1);

    // Store the player who won the coin toss
    FirstPlayerIndex = StartingPlayer;

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
    // Remove any existing coin toss widget to prevent overlaps
    if (CoinTossWidget && CoinTossWidget->IsInViewport())
    {
        CoinTossWidget->RemoveFromParent();
        CoinTossWidget = nullptr;
    }

    if (!CoinTossWidgetClass)
        return;

    // Get the current turn message from the game instance
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (!GameInstance)
        return;

    FString CoinTossMessage = GameInstance->GetTurnMessage();

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

void ATBS_GameMode::ResetGame()
{
    // Make sure we're not in the middle of something important
    if (CurrentPhase == EGamePhase::SETUP)
    {
        // Hide unit selection UI if visible
        HideUnitSelectionUI();
    }

    // Reset the game for a new round without declaring a winner
    // Use -1 to indicate no specific winner for this reset
    ResetForNewRound(-1);
}

FString ATBS_GameMode::WinningPlayerMessage(int32 WinnerIndex)
{
    if (WinnerIndex == 0)
    {
        return FString::Printf(TEXT("Human Player Won!"));
    }
    
    return FString::Printf(TEXT("AI Player Won!"));

}

void ATBS_GameMode::DebugForceGameplay()
{
    UnitsPlaced = NumberOfPlayers * UnitsPerPlayer;
    StartGameplayPhase();
}

void ATBS_GameMode::StartPlacementPhase(int32 StartingPlayer)
{
    // Clear UI first
    HideUnitSelectionUI();

    // Reset the current player and phase
    CurrentPlayer = StartingPlayer;
    CurrentPhase = EGamePhase::SETUP;

    // Notify the player it's their turn to place
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];

        // Call the OnPlacement function
        ITBS_PlayerInterface::Execute_OnPlacement(PlayerActor);

        // If it's human player's turn, show UI with slight delay to ensure other UIs are cleared
        if (CurrentPlayer == 0)
        {
            // Timer to delay showing the UI slightly
            FTimerHandle ShowUITimerHandle;
            GetWorldTimerManager().SetTimer(ShowUITimerHandle, [this]()
                {
                    ShowUnitSelectionUI(true);
                }, 0.2f, false);
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player index not valid"));
    }
}

void ATBS_GameMode::StartGameplayPhase()
{
    // Explicitly set the phase to GAMEPLAY
    CurrentPhase = EGamePhase::GAMEPLAY;

    // Reset any placement-related flags
    UnitsPlaced = NumberOfPlayers * UnitsPerPlayer;

    // Reset all placement flags to avoid confusion
    for (int32 i = 0; i < BrawlerPlaced.Num(); i++)
    {
        BrawlerPlaced[i] = true;
        SniperPlaced[i] = true;
    }

    // Reset to the player who won the coin toss for the gameplay phase
    CurrentPlayer = FirstPlayerIndex;

    // Clear any UI widgets that might be causing interference
    if (UnitSelectionWidget && UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Reset player actions state
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
        }
    }

    // Notify the player it's their turn (with delay to ensure everything is set up)
    FTimerHandle TurnStartTimerHandle;
    GetWorldTimerManager().SetTimer(TurnStartTimerHandle, [this]()
        {
            if (Players.IsValidIndex(CurrentPlayer))
            {
                ITBS_PlayerInterface::Execute_OnTurn(Players[CurrentPlayer]);
            }
        }, 0.5f, false);
}
void ATBS_GameMode::EndTurn()
{
    // Let the current player know their turn is ending
    if (Players.IsValidIndex(CurrentPlayer))
    {
        ITBS_PlayerInterface::Execute_SetTurnState(Players[CurrentPlayer], false);
    }

    // Skip to next player
    CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;

    // Check if the game is over
    if (bIsGameOver)
    {
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
    }
}

bool ATBS_GameMode::PlaceUnit(EUnitType Type, int32 GridX, int32 GridY, int32 PlayerIndex)
{
    // Early validation checks
    if (!GameGrid || CurrentPhase != EGamePhase::SETUP || PlayerIndex != CurrentPlayer)
    {
        return false;
    }

    // Check if player already placed this unit type
    if ((Type == EUnitType::BRAWLER && BrawlerPlaced[PlayerIndex]) ||
        (Type == EUnitType::SNIPER && SniperPlaced[PlayerIndex]))
    {
        return false;
    }

    // Validate tile
    FVector2D Position(GridX, GridY);
    if (!GameGrid->TileMap.Contains(Position))
    {
        return false;
    }

    ATile* Tile = GameGrid->TileMap[Position];
    if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY)
    {
        return false;
    }

    // Spawn unit - ensure both classes are set
    if (!BrawlerClass || !SniperClass)
    {
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
        return false;
    }

    // Initialize unit
    NewUnit->SetOwnerID(PlayerIndex);
    NewUnit->InitializePosition(Tile);

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
    }

    // Mark unit type as placed
    if (Type == EUnitType::BRAWLER)
        BrawlerPlaced[PlayerIndex] = true;
    else if (Type == EUnitType::SNIPER)
        SniperPlaced[PlayerIndex] = true;

    // Increment units placed
    UnitsPlaced++;

    // Check if all units are placed
    if (UnitsPlaced >= NumberOfPlayers * UnitsPerPlayer)
    {
        // Add a delay before starting gameplay phase
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

    // Count each player's remaining units
    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), PlayerUnits);

    // Reset counters
    UnitsRemaining.SetNum(NumberOfPlayers);
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        UnitsRemaining[i] = 0;
    }

    // Count units by player
    for (AActor* Actor : PlayerUnits)
    {
        AUnit* Unit = Cast<AUnit>(Actor);
        if (Unit && !Unit->IsDead())
        {
            int32 OwnerID = Unit->GetOwnerID();
            if (OwnerID >= 0 && OwnerID < NumberOfPlayers)
            {
                UnitsRemaining[OwnerID]++;
            }
        }
    }

    // Check if any player has lost all units
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        if (UnitsRemaining[i] == 0)
        {
            bGameOver = true;
            WinningPlayer = (i + 1) % NumberOfPlayers; // Other player wins
        }
    }

    if (bGameOver && !bIsGameOver)
    {
        // Call PlayerWon with a slight delay to ensure all game state updates
        FTimerHandle GameOverTimerHandle;
        int32 FinalWinner = WinningPlayer;
        GetWorldTimerManager().SetTimer(GameOverTimerHandle, [this, FinalWinner]()
            {
                PlayerWon(FinalWinner);
            }, 0.5f, false);
    }

    return bGameOver;
}

void ATBS_GameMode::NotifyUnitDestroyed(int32 PlayerIndex)
{
    // Decrement the player's unit count
    if (UnitsRemaining.IsValidIndex(PlayerIndex))
    {
        UnitsRemaining[PlayerIndex]--;

        // Check if the game is over with a slight delay to ensure all damage is processed
        FTimerHandle CheckGameOverTimerHandle;
        GetWorldTimerManager().SetTimer(CheckGameOverTimerHandle, [this]()
            {
                CheckGameOver();
            }, 0.1f, false);
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
    if (CurrentPhase == EGamePhase::ROUND_END)
    {
        return;
    }

    // Set phase to ROUND_END to prevent multiple calls
    CurrentPhase = EGamePhase::ROUND_END;
    bIsGameOver = true;

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

    // Get game instance to update the score and set the winner
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Update score
        if (PlayerIndex == 0)
        {
            GameInstance->IncrementScoreHumanPlayer();
        }
        else
        {
            GameInstance->IncrementScoreAiPlayer();
        }

        // Set the winner in the game instance
        GameInstance->SetWinner(PlayerIndex);

        // Show end game message
        FString WinnerName = (PlayerIndex == 0) ? TEXT("Human Player") : TEXT("AI Player");
        FString EndGameMessage = FString::Printf(TEXT("GAME OVER! %s has WON the game! (Score: Human %d - AI %d)"),
            *WinnerName, GameInstance->GetScoreHumanPlayer(), GameInstance->GetScoreAiPlayer());

        GameInstance->SetTurnMessage(EndGameMessage);
    }
}

void ATBS_GameMode::ResetForNewRound(int32 WinnerIndex)
{
    // Get the game instance to update the round counter and score
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Reset the game without changing scores
        // Increment round counter
        GameInstance->IncrementRound();

        // Clear move history for the new round
        GameInstance->ClearMoveHistory();

        // Reset winner tracking
        GameInstance->ResetWinner();

        // Display new round message
        FString NewRoundMessage = FString::Printf(TEXT("Starting Round %d"), GameInstance->GetCurrentRound());
        GameInstance->SetTurnMessage(NewRoundMessage);
    }

    // Clear any UI widgets immediately
    HideUnitSelectionUI();

    if (CoinTossWidget && CoinTossWidget->IsInViewport())
    {
        CoinTossWidget->RemoveFromParent();
        CoinTossWidget = nullptr;
    }

    // Reset game variables immediately
    bIsGameOver = false;
    UnitsPlaced = 0;
    CurrentPhase = EGamePhase::NONE;

    // Reset unit placement tracking
    BrawlerPlaced.Init(false, NumberOfPlayers);
    SniperPlaced.Init(false, NumberOfPlayers);

    // Start a new round with slight delay
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            // Reset remaining units count
            UnitsRemaining.SetNum(NumberOfPlayers);
            for (int32 i = 0; i < NumberOfPlayers; i++)
            {
                UnitsRemaining[i] = UnitsPerPlayer;
            }

            // Remove all units from the grid
            TArray<AActor*> AllUnits;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);
            for (AActor* UnitActor : AllUnits)
            {
                if (UnitActor)
                {
                    // Get the unit reference
                    AUnit* Unit = Cast<AUnit>(UnitActor);
                    if (Unit)
                    {
                        // Clean up its tile
                        ATile* CurrentTile = Unit->GetCurrentTile();
                        if (CurrentTile)
                        {
                            CurrentTile->SetTileStatus(AGrid::NOT_ASSIGNED, ETileStatus::EMPTY);
                            CurrentTile->SetOccupyingUnit(nullptr);
                        }
                    }
                    // Destroy the unit
                    UnitActor->Destroy();
                }
            }

            // Reset the grid and regenerate obstacles
            if (GameGrid)
            {
                GameGrid->ResetGrid();
                // Spawn new obstacles for the next round
                SpawnObstaclesWithConnectivity();
            }

            // Reset player states
            for (AActor* PlayerActor : Players)
            {
                if (ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerActor))
                {
                    HumanPlayer->ResetActionState();
                    HumanPlayer->ClearCurrentPlacementTile();
                }
                else if (ATBS_NaiveAI* AIPlayer = Cast<ATBS_NaiveAI>(PlayerActor))
                {
                    AIPlayer->ResetActionState();
                }
            }

            // Add an additional delay before starting the new placement phase to ensure UI is reset
            FTimerHandle StartPlacementTimerHandle;
            GetWorldTimerManager().SetTimer(StartPlacementTimerHandle, [this]()
                {
                    // Start a new round with coin toss
                    int32 StartingPlayer = SimulateCoinToss();
                    StartPlacementPhase(StartingPlayer);
                }, 0.5f, false);

        }, 1.0f, false);
}

void ATBS_GameMode::RecordMove(int32 PlayerIndex, FString UnitType, FString ActionType,
    FVector2D FromPosition, FVector2D ToPosition, int32 Damage)
{
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

    GameInstance->AddMoveToHistory(PlayerIndex, UnitType, ActionType, FromPosition, ToPosition, Damage);

}

bool ATBS_GameMode::WouldBreakConnectivity(int32 GridX, int32 GridY)
{
    if (!GameGrid)
    {
        return true;
    }

    // Get the tile at the specified position
    FVector2D Position(GridX, GridY);

    if (!GameGrid->TileMap.Contains(Position))
    {
        return true;
    }

    ATile* Tile = GameGrid->TileMap[Position];

    // Skip if the tile is already an obstacle or occupied
    if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY || Tile->IsObstacle())
    {
        return true;
    }

    // Temporarily mark this tile as an obstacle
    ETileStatus OriginalStatus = Tile->GetTileStatus();
    int32 OriginalOwner = Tile->GetOwner();

    Tile->SetTileStatus(-2, ETileStatus::OCCUPIED);

    // Check if the grid is still connected
    bool IsConnected = GameGrid->ValidateConnectivity();

    // Restore the tile's original state
    Tile->SetTileStatus(OriginalOwner, OriginalStatus);

    // Return true if placing an obstacle here would break connectivity
    return !IsConnected;
}

void ATBS_GameMode::SpawnObstaclesWithConnectivity()
{
    // Validate grid existence
    if (!GameGrid)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error: GameGrid is null"));
        return;
    }

    // Clear any existing obstacles first
    for (ATile* Tile : GameGrid->TileArray)
    {
        if (Tile && (Tile->IsObstacle() || Tile->GetOwner() == -2))
        {
            Tile->SetTileStatus(AGrid::NOT_ASSIGNED, ETileStatus::EMPTY);
        }
    }

    // Calculate target obstacles
    int32 TotalCells = GridSize * GridSize;
    int32 TargetObstacles = FMath::RoundToInt((ObstaclePercentage / 100.0f) * TotalCells);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("Attempting to place %d obstacles (%.1f%%)"),
            TargetObstacles, ObstaclePercentage));

    // Create an array of all valid tile positions
    TArray<FVector2D> ValidPositions;
    for (int32 x = 1; x < GridSize - 1; x++)
    {
        for (int32 y = 1; y < GridSize - 1; y++)
        {
            ValidPositions.Add(FVector2D(x, y));
        }
    }

    // Shuffle for randomness
    for (int32 i = ValidPositions.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = FMath::RandRange(0, i);
        if (i != SwapIndex)
        {
            ValidPositions.Swap(i, SwapIndex);
        }
    }

    // First pass: Just place obstacles directly without connectivity checks
    int32 PlacedObstacles = 0;
    for (int32 i = 0; i < ValidPositions.Num() && PlacedObstacles < TargetObstacles; i++)
    {
        FVector2D Pos = ValidPositions[i];

        if (!GameGrid->TileMap.Contains(Pos))
            continue;

        ATile* Tile = GameGrid->TileMap[Pos];

        // Skip invalid tiles
        if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY ||
            Tile->GetOccupyingUnit() || Tile->IsObstacle())
            continue;

        // Place obstacle directly
        Tile->SetAsObstacle();
        PlacedObstacles++;
    }

    // Now check connectivity and fix if needed
    bool IsConnected = GameGrid->ValidateConnectivity();
    if (!IsConnected)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Fixing connectivity by removing obstacles..."));

        // Store all obstacles in an array
        TArray<ATile*> Obstacles;
        for (ATile* Tile : GameGrid->TileArray)
        {
            if (Tile && Tile->IsObstacle())
            {
                Obstacles.Add(Tile);
            }
        }

        // Shuffle so we remove random obstacles
        for (int32 i = Obstacles.Num() - 1; i > 0; i--)
        {
            int32 SwapIndex = FMath::RandRange(0, i);
            if (i != SwapIndex)
            {
                Obstacles.Swap(i, SwapIndex);
            }
        }

        // Remove obstacles until connectivity is restored
        for (ATile* Obstacle : Obstacles)
        {
            if (!Obstacle)
                continue;

            // Remove obstacle
            Obstacle->SetTileStatus(AGrid::NOT_ASSIGNED, ETileStatus::EMPTY);

            // Check if connectivity is restored
            IsConnected = GameGrid->ValidateConnectivity();
            if (IsConnected)
                break;
        }
    }

    // Final count
    int32 FinalObstacles = 0;
    for (ATile* Tile : GameGrid->TileArray)
    {
        if (Tile && Tile->IsObstacle())
        {
            FinalObstacles++;
        }
    }

    float FinalPercentage = (FinalObstacles * 100.0f) / TotalCells;

    // Log final stats
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("Final obstacles: %d/%d (%.1f%%)"),
            FinalObstacles, TargetObstacles, FinalPercentage));
}