// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Brawler.h"
#include "Sniper.h"
#include "TBS_HumanPlayer.h"
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
}

void ATBS_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Find the human player
    ATBS_HumanPlayer* HumanPlayer = GetWorld()->GetFirstPlayerController()->GetPawn<ATBS_HumanPlayer>();
    if (!IsValid(HumanPlayer))
    {
        UE_LOG(LogTemp, Error, TEXT("No player pawn of type ATBS_HumanPlayer was found."));
        return;
    }

    // Spawn Grid
    if (GridClass != nullptr)
    {
        GameGrid = GetWorld()->SpawnActor<AGrid>(GridClass);
        GameGrid->Size = GridSize;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Game Field is null"));
    }

    float CameraPosX = ((GameGrid->TileSize * GridSize) + ((GridSize - 1) * GameGrid->TileSize * GameGrid->CellPadding)) * 0.5f;
    float Zposition = 1000.0f;
    FVector CameraPos(CameraPosX, CameraPosX, Zposition);
    HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

    //// Find all players (Human and AI) -> added in case of more players
    //TArray<AActor*> FoundPlayers;
    //UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTBS_PlayerInterface::StaticClass(), FoundPlayers);

    //// Add them to our Players array
    //for (AActor* Actor : FoundPlayers)
    //{
    //    Players.Add(Actor);
    //}

    // Human player = 0
    Players.Add(HumanPlayer);
    // Random Player
//   auto* AI = GetWorld()->SpawnActor<ATTT_RandomPlayer>(FVector(), FRotator());

    // Set initial values for units per player
    UnitsRemaining.SetNum(NumberOfPlayers);
    for (int32 i = 0; i < NumberOfPlayers; i++)
    {
        UnitsRemaining[i] = UnitsPerPlayer;
    }

    // Start the game with a coin toss
    int32 StartingPlayer = SimulateCoinToss();
    StartPlacementPhase(StartingPlayer);
}

// Binary Coin Toss
int32 ATBS_GameMode::SimulateCoinToss()
{
    // Randomly determine starting player (0 or 1)
    return FMath::RandRange(0, 1);
}

// Placement Phase
void ATBS_GameMode::StartPlacementPhase(int32 StartingPlayer)
{
    CurrentPlayer = StartingPlayer;
    CurrentPhase = EGamePhase::SETUP;       // Setup phase

    // Notify the first player it's their turn to place units
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];
        
        // Get the interface pointer
        ITBS_PlayerInterface* PlayerInterface = Cast<ITBS_PlayerInterface>(PlayerActor);
        // Call the OnPlacement function
        PlayerInterface->OnPlacement();
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