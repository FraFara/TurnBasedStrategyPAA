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

    // Set default player controller class
    PlayerControllerClass = ATBS_PlayerController::StaticClass();

    // Set default pawn class
    DefaultPawnClass = ATBS_HumanPlayer::StaticClass();
}

void ATBS_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Find the human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("No Player Controller found"));
        return;
    }

    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
    if (!HumanPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempting to spawn HumanPlayer"));

        // If no pawn exists, try to spawn one
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = PlayerController;
        HumanPlayer = GetWorld()->SpawnActor<ATBS_HumanPlayer>(ATBS_HumanPlayer::StaticClass(), SpawnParams);

        if (HumanPlayer)
        {
            PlayerController->Possess(HumanPlayer);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn HumanPlayer"));
            return;
        }
    }

    // Spawn Grid
    if (GridClass != nullptr)
    {
        GameGrid = GetWorld()->SpawnActor<AGrid>(GridClass);
        if (GameGrid)
        {
            GameGrid->Size = GridSize;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn grid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GridClass is null"));
    }

    float CameraPosX = ((GameGrid->TileSize * GridSize) + ((GridSize - 1) * GameGrid->TileSize * GameGrid->CellPadding)) * 0.5f;
    float Zposition = 3000.0f;
    FVector CameraPos(CameraPosX, CameraPosX, Zposition);
    HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

    // Find all players (Human and AI) -> added in case of more players
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTBS_PlayerInterface::StaticClass(), FoundPlayers);

    // Clear Players array before adding
    Players.Empty();

    // Add human player (index 0)
    Players.Add(HumanPlayer);

    // Add AI player (index 1)
    auto* AI = GetWorld()->SpawnActor<ATBS_NaiveAI>(FVector(), FRotator());
    Players.Add(AI);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Players added: %d"), Players.Num()));

    // Aggiungere eventuale altra ia

    // Set initial values for units per player
    UnitsRemaining.SetNum(NumberOfPlayers);
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        UnitsRemaining[i] = UnitsPerPlayer;
    }

    // Add debug messages for game start
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Game Starting - Coin Toss"));

    // Place some obstacles 
    SpawnObstacles();

    // Start the game with a coin toss
    int32 StartingPlayer = SimulateCoinToss();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Coin Toss Result: Player %d starts"), StartingPlayer));

    StartPlacementPhase(StartingPlayer);
}

void ATBS_GameMode::ShowUnitSelectionUI()
{
    if (!UnitSelectionWidgetClass)
        return;

    // Create the widget if it doesn't exist
    if (!UnitSelectionWidget)
    {
        UnitSelectionWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), UnitSelectionWidgetClass);
    }

    if (UnitSelectionWidget && !UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->AddToViewport();
    }
}

void ATBS_GameMode::HideUnitSelectionUI()
{
    if (UnitSelectionWidget && UnitSelectionWidget->IsInViewport())
    {
        UnitSelectionWidget->RemoveFromParent();
    }
}

void ATBS_GameMode::OnUnitTypeSelected(EUnitType SelectedType)
{
    // Hide the selection UI
    HideUnitSelectionUI();

    // Get the current human player
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(PlayerController->GetPawn());
        if (HumanPlayer)
        {
            // Set the selected unit type
            if (SelectedType == EUnitType::BRAWLER)
            {
                HumanPlayer->SelectBrawlerForPlacement();
            }
            else if (SelectedType == EUnitType::SNIPER)
            {
                HumanPlayer->SelectSniperForPlacement();
            }
        }
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

// Modify the StartPlacementPhase function to show UI for human player
void ATBS_GameMode::StartPlacementPhase(int32 StartingPlayer)
{
    // Keep existing code
    CurrentPlayer = StartingPlayer;
    CurrentPhase = EGamePhase::SETUP;

    // Notify the player it's their turn to place
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];

        // Get the interface pointer
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);

        // Call the OnPlacement function
        PlayerInterface->OnPlacement();

        // Show unit selection UI if it's the human player's turn (player 0)
        if (CurrentPlayer == 0)
        {
            ShowUnitSelectionUI();
        }
    }
}

void ATBS_GameMode::StartGameplayPhase()
{
    CurrentPhase = EGamePhase::GAMEPLAY;        // Gameplay phase

    // Reset to the player who won the coin toss
    if (Players.IsValidIndex(CurrentPlayer))
    {
        // Reset all units for new turn
        TArray<AActor*> AllUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

        for (AActor* Actor : AllUnits)
        {
            AUnit* Unit = Cast<AUnit>(Actor);

            Unit->ResetTurn();

        }

        // Notify the player it's their turn
        AActor* PlayerActor = Players[CurrentPlayer];

        // Get the interface pointer
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);

        // Call the OnTurn function
        PlayerInterface->OnTurn();
    }
}

void ATBS_GameMode::EndTurn()
{
    // Skip to next player
    CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;

    // Check if the game is over
    if (bIsGameOver)
    {
        return;
    }

    // Resets all units for the new player's turn
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

    // Notify the next player it's their turn
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];

        // Get the interface pointer
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);

        // Call the OnTurn function
        PlayerInterface->OnTurn();
    }
}

bool ATBS_GameMode::PlaceUnit(EUnitType Type, int32 GridX, int32 GridY, int32 PlayerIndex)
{
    if (!GameGrid || CurrentPhase != EGamePhase::SETUP || PlayerIndex != CurrentPlayer)
    {
        return false;
    }

    // Checks if the tile is valid and empty
    if (!GameGrid->TileMap.Contains(FVector2D(GridX, GridY)))
    {
        return false;
    }

    ATile* Tile = GameGrid->TileMap[FVector2D(GridX, GridY)];
    if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY)
    {
        return false;
    }

    // Spawns the unit
    AUnit* NewUnit = nullptr;
    FVector SpawnLocation = Tile->GetActorLocation() + FVector(0, 0, 10.0f); // Spawns the unit slightly above the tiles

    if (Type == EUnitType::BRAWLER && BrawlerClass)
    {
        NewUnit = GetWorld()->SpawnActor<AUnit>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator);
    }
    else if (Type == EUnitType::SNIPER && SniperClass)
    {
        NewUnit = GetWorld()->SpawnActor<AUnit>(SniperClass, SpawnLocation, FRotator::ZeroRotator);
    }

    if (NewUnit)
    {
        // Set up the unit
        NewUnit->SetOwner(PlayerIndex);
        NewUnit->InitializePosition(Tile);

        // Increment units placed
        UnitsPlaced++;

        // Switch to next player if not the last unit
        if (UnitsPlaced < NumberOfPlayers * UnitsPerPlayer)
        {
            // Switch to the other player if we've placed one unit
            CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;

            if (Players.IsValidIndex(CurrentPlayer))
            {
                AActor* PlayerActor = Players[CurrentPlayer];

                // Get the interface pointer
                ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);
                if (PlayerInterface)
                {
                    // Call the OnPlacement function
                    PlayerInterface->OnPlacement();
                }

            }

        }
        else
        {
            // All units placed, start gameplay
            StartGameplayPhase();
        }

        return true;
    }

    return false;
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
                        PlayerInterface->OnWin();
                    }
                    else
                    {
                        // Call the OnLose function
                        PlayerInterface->OnLose();
                    }
                }
            }
        }
    }
}

void ATBS_GameMode::SpawnObstacles()
{
    if (ObstaclePercentage <= 0.0f)
        return;

    // Calculate number of obstacles to place
    int32 TotalCells = GridSize * GridSize;
    int32 ObstaclesToPlace = FMath::RoundToInt((ObstaclePercentage / 100.0f) * TotalCells);

    // Simple random placement for now
    for (int32 i = 0; i < ObstaclesToPlace; i++)
    {
        int32 RandomX = FMath::RandRange(0, GridSize - 1);
        int32 RandomY = FMath::RandRange(0, GridSize - 1);

        if (GameGrid->TileMap.Contains(FVector2D(RandomX, RandomY)))
        {
            ATile* Tile = GameGrid->TileMap[FVector2D(RandomX, RandomY)];
            if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY)
            {
                // Mark as obstacles
                // For now, we can just make it occupied with a special owner ID
                Tile->SetTileStatus(-2, ETileStatus::OCCUPIED); // -2 for obstacles

                // Specific mesh or material for obstacles here
            }
        }
    }
}


void ATBS_GameMode::RecordMove(int32 PlayerIndex, FString UnitType, FString ActionType,
    FVector2D FromPosition, FVector2D ToPosition, int32 Damage)
{
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

    GameInstance->AddMoveToHistory(PlayerIndex, UnitType, ActionType, FromPosition, ToPosition, Damage);

}