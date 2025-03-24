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
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn grid"));
            return;
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GridClass is null"));
        return;
    }

    // Find/Initialize the human player
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

    // Debug message for game start
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Game Starting - Choose AI Difficulty"));
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
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Player Controller found"));
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

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("AI Selection UI shown"));
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
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Easy AI (Random) selected"));

    // Hide the selection UI
    HideAISelectionUI();

    // Update GameInstance with a message
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->SetTurnMessage(TEXT("Easy AI (Random) selected - Starting game..."));
    }

    // Proceed with game initialization
    InitializeGame();
}

void ATBS_GameMode::OnHardAISelected()
{
    bUseSmartAI = true;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Hard AI (Smart) selected"));

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

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("%s AI selected"), bIsHardAI ? TEXT("Smart") : TEXT("Naive")));

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
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn grid"));
            return;
        }
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
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = PlayerController;
        HumanPlayer = GetWorld()->SpawnActor<ATBS_HumanPlayer>(ATBS_HumanPlayer::StaticClass(), SpawnParams);

        if (HumanPlayer)
        {
            PlayerController->Possess(HumanPlayer);
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
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("%s AI Player added successfully"), bUseSmartAI ? TEXT("Smart") : TEXT("Naive")));
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

    // Place obstacles
    SpawnObstacles();

    // Start the game with a coin toss
    int32 StartingPlayer = SimulateCoinToss();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Coin Toss Result: Player %d starts"), StartingPlayer));

    StartPlacementPhase(StartingPlayer);
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
    // First, remove any existing coin toss widget to prevent overlaps
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

void ATBS_GameMode::ResetGame()
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Game Reset Requested"));

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
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FORCING GAMEPLAY PHASE START"));
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

    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, TEXT("StartPlacementPhase called"));

    // Notify the player it's their turn to place
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];

        // Call the OnPlacement function
        ITBS_PlayerInterface::Execute_OnPlacement(PlayerActor);

        // If it's human player's turn, show UI with slight delay to ensure other UIs are cleared
        if (CurrentPlayer == 0)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Human Player's turn for placement"));

            // Timer to delay showing the UI slightly
            FTimerHandle ShowUITimerHandle;
            GetWorldTimerManager().SetTimer(ShowUITimerHandle, [this]()
                {
                    ShowUnitSelectionUI(true);
                }, 0.2f, false);
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
                FString::Printf(TEXT("AI's turn for placement (Player %d)"), CurrentPlayer));
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

    // Reset to the player who won the coin toss for the gameplay phase
    CurrentPlayer = FirstPlayerIndex;

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

//bool ATBS_GameMode::CheckGameOver()
//{
//    bool bGameOver = false;
//    int32 WinningPlayer = -1;
//
//    // Reset unit count
//    UnitsRemaining.SetNum(NumberOfPlayers);
//    for (int32 i = 0; i < NumberOfPlayers; i++)
//    {
//        UnitsRemaining[i] = 0;
//    }
//
//    // Check each player's remaining units
//    for (int32 i = 0; i < NumberOfPlayers; i++)
//    {
//        TArray<AActor*> PlayerUnits;
//        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), PlayerUnits);
//
//        int32 RemainingUnits = 0;
//        for (AActor* Actor : PlayerUnits)
//        {
//            AUnit* Unit = Cast<AUnit>(Actor);
//            if (Unit && Unit->GetOwnerID() == i)
//            {
//                RemainingUnits++;
//            }
//        }
//
//        UnitsRemaining[i] = RemainingUnits;
//
//        // Detailed debug logging to understand unit counts
//        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
//            FString::Printf(TEXT("Player %d has %d units remaining"), i, RemainingUnits));
//
//        // If a player has no units, the other player wins
//        if (RemainingUnits == 0)
//        {
//            bGameOver = true;
//            WinningPlayer = (i + 1) % NumberOfPlayers; // Other player wins
//
//            // Add more explicit messaging
//            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
//                FString::Printf(TEXT("Player %d has no units left! Player %d wins this round!"),
//                    i, WinningPlayer));
//        }
//    }
//
//    if (bGameOver && !bIsGameOver)
//    {
//        bIsGameOver = true;
//        PlayerWon(WinningPlayer);
//    }
//
//    return bGameOver;
//}

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
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("Player %d has %d units remaining"), i, UnitsRemaining[i]));

        if (UnitsRemaining[i] == 0)
        {
            bGameOver = true;
            WinningPlayer = (i + 1) % NumberOfPlayers; // Other player wins

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                FString::Printf(TEXT("Player %d has no units left! Player %d WINS!"),
                    i, WinningPlayer));
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

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
            FString::Printf(TEXT("Player %d lost a unit! Units remaining: %d"),
                PlayerIndex, UnitsRemaining[PlayerIndex]));

        // Check if the game is over with a slight delay to ensure all damage is processed
        FTimerHandle CheckGameOverTimerHandle;
        GetWorldTimerManager().SetTimer(CheckGameOverTimerHandle, [this]()
            {
                CheckGameOver();
            }, 0.1f, false);
    }
}

//void ATBS_GameMode::NotifyUnitDestroyed(int32 PlayerIndex)
//{
//    // Decrement the player's unit count
//    if (UnitsRemaining.IsValidIndex(PlayerIndex))
//    {
//        UnitsRemaining[PlayerIndex]--;
//
//        // Check if the game is over
//        CheckGameOver();
//    }
//}

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
    // Debug message to confirm function is called
    GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green,
        FString::Printf(TEXT("GAME OVER! Player %d has WON the game!"), PlayerIndex));

    // Make sure we aren't calling this multiple times
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

        // Show on-screen message
        GEngine->AddOnScreenDebugMessage(-1, 30.f,
            PlayerIndex == 0 ? FColor::Green : FColor::Red,
            EndGameMessage);
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
                SpawnObstacles();
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

    // Reset obstacle count tracking for the attempt
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
        FString::Printf(TEXT("Attempting to place %d obstacles (%.1f%% of %d cells)"),
            ObstaclesToPlace, ObstaclePercentage, TotalCells));

    // Validate grid map
    if (GameGrid->TileMap.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot spawn obstacles: TileMap is empty"));
        return;
    }

    // Track actual obstacles placed
    int32 PlacedObstacles = 0;

    // Make a copy of the tiles array to avoid modifying it while iterating
    TArray<ATile*> AvailableTiles;
    for (ATile* Tile : GameGrid->TileArray)
    {
        if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY && !Tile->GetOccupyingUnit())
        {
            AvailableTiles.Add(Tile);
        }
    }

    // Shuffle the available tiles (Fisher-Yates algorithm)
    int32 LastIndex = AvailableTiles.Num() - 1;
    for (int32 i = LastIndex; i > 0; i--)
    {
        int32 SwapIndex = FMath::RandRange(0, i);
        if (i != SwapIndex)
        {
            AvailableTiles.Swap(i, SwapIndex);
        }
    }

    // Clear progress message
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Shuffled tile array for obstacle placement"));

    // Place obstacles by iterating through the shuffled array
    for (int32 i = 0; i < AvailableTiles.Num() && PlacedObstacles < ObstaclesToPlace; i++)
    {
        ATile* Tile = AvailableTiles[i];

        // Verify the tile is still available (belt and suspenders approach)
        if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY && !Tile->GetOccupyingUnit())
        {
            // Try to mark the tile as an obstacle
            Tile->SetAsObstacle();

            // Verify that the tile was successfully marked
            if (Tile->GetTileStatus() == ETileStatus::OCCUPIED && Tile->GetOwner() == -2)
            {
                PlacedObstacles++;

                // Debug each successful obstacle placement
                if (PlacedObstacles % 10 == 0 || PlacedObstacles == ObstaclesToPlace)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                        FString::Printf(TEXT("Placed %d/%d obstacles"), PlacedObstacles, ObstaclesToPlace));
                }
            }
            else
            {
                // Log if a tile couldn't be marked as an obstacle
                GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange,
                    FString::Printf(TEXT("Failed to mark tile at (%f,%f) as obstacle"),
                        Tile->GetGridPosition().X, Tile->GetGridPosition().Y));
            }
        }
    }

    // Final verification pass - check all tiles and make sure obstacles are properly marked
    int32 verifiedObstacles = 0;
    for (ATile* Tile : GameGrid->TileArray)
    {
        if (Tile && Tile->GetTileStatus() == ETileStatus::OCCUPIED && Tile->GetOwner() == -2)
        {
            verifiedObstacles++;
        }
    }

    // Final log
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
        FString::Printf(TEXT("Completed: Placed and verified %d/%d obstacles (%.1f%%)"),
            verifiedObstacles, ObstaclesToPlace, (float)verifiedObstacles / ObstaclesToPlace * 100.0f));
}


void ATBS_GameMode::RecordMove(int32 PlayerIndex, FString UnitType, FString ActionType,
    FVector2D FromPosition, FVector2D ToPosition, int32 Damage)
{
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

    GameInstance->AddMoveToHistory(PlayerIndex, UnitType, ActionType, FromPosition, ToPosition, Damage);

}