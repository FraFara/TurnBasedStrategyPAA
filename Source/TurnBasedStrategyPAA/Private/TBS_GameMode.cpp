// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Brawler.h"
#include "Sniper.h"
#include "TBS_PlayerInterface.h"

ATBS_GameMode::ATBS_GameMode()
{
    // Default values
    NumberOfPlayers = 2;
    UnitsPerPlayer = 2;
    CurrentPlayer = 0;
    CurrentPhase = EGamePhase::NONE;
    UnitsPlaced = 0;
    bIsGameOver = false;
    ObstaclePercentage = 10.0f;     // I picked this as default percentage
}

void ATBS_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Find the grid
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        GameGrid = Cast<AGrid>(FoundActors[0]);
    }

    // Find all players (Human and AI)
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UTBS_PlayerInterface::StaticClass(), FoundPlayers);

    // Add them to our Players array
    for (AActor* Actor : FoundPlayers)
    {
        Players.Add(Actor);
    }

    // Set initial values for units remaining
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
void ATBS_GameMode::StartPlacementPhase(int32 StartingPlayerIndex)
{
    CurrentPlayer = StartingPlayerIndex;
    CurrentPhase = EGamePhase::SETUP;       // Setup phase

    // Notify the first player it's their turn to place units
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];
        if (PlayerActor && PlayerActor->GetClass()->ImplementsInterface(UTBS_PlayerInterface::StaticClass()))
        {
            ITBS_PlayerInterface::Execute_OnPlacementTurn(PlayerActor);
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
            if (Unit)
            {
                Unit->ResetTurn();
            }
        }

        // Notify the player it's their turn
        AActor* PlayerActor = Players[CurrentPlayer];
        if (PlayerActor && PlayerActor->GetClass()->ImplementsInterface(UTBS_PlayerInterface::StaticClass()))
        {
            ITBS_PlayerInterface::Execute_OnTurn(PlayerActor);
        }
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

    // Notify the next player it's their turn
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Players[CurrentPlayer];
        if (PlayerActor && PlayerActor->GetClass()->ImplementsInterface(UTBS_PlayerInterface::StaticClass()))
        {
            ITBS_PlayerInterface::Execute_OnTurn(PlayerActor);
        }
    }
}

bool ATBS_GameMode::PlaceUnit(FString UnitType, int32 GridX, int32 GridY, int32 PlayerIndex)
{
    if (!GameGrid || CurrentPhase != EGamePhase::SETUP || PlayerIndex != CurrentPlayer)
    {
        return false;
    }

    // Check if the tile is valid and empty
    if (!GameGrid->TileMap.Contains(FVector2D(GridX, GridY)))
    {
        return false;
    }

    ATile* Tile = GameGrid->TileMap[FVector2D(GridX, GridY)];
    if (!Tile || Tile->GetTileStatus() != ETileStatus::EMPTY)
    {
        return false;
    }

    // Spawn the unit
    AUnit* NewUnit = nullptr;
    FVector SpawnLocation = Tile->GetActorLocation() + FVector(0, 0, 20.0f);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (UnitType.Equals("Brawler") && BrawlerClass)
    {
        NewUnit = GetWorld()->SpawnActor<AUnit>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    }
    else if (UnitType.Equals("Sniper") && SniperClass)
    {
        NewUnit = GetWorld()->SpawnActor<AUnit>(SniperClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
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
            if (UnitsPlaced % 2 == 1)
            {
                CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;
                if (Players.IsValidIndex(CurrentPlayer))
                {
                    AActor* PlayerActor = Players[CurrentPlayer];
                    if (PlayerActor && PlayerActor->GetClass()->ImplementsInterface(UTBS_PlayerInterface::StaticClass()))
                    {
                        ITBS_PlayerInterface::Execute_OnPlacementTurn(PlayerActor);
                    }
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
            if (PlayerActor && PlayerActor->GetClass()->ImplementsInterface(UTBS_PlayerInterface::StaticClass()))
            {
                if (i == PlayerIndex)
                {
                    ITBS_PlayerInterface::Execute_OnWin(PlayerActor);
                }
                else
                {
                    ITBS_PlayerInterface::Execute_OnLose(PlayerActor);
                }
            }
        }
    }
}