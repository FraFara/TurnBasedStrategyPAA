// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_SmartAI.h"
#include "Kismet/GameplayStatics.h"
#include "Grid.h"
#include "TBS_GameMode.h"
#include "TBS_GameInstance.h"
#include "EngineUtils.h"
#include "Sniper.h"
#include "Brawler.h"

// Sets default values
ATBS_SmartAI::ATBS_SmartAI()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Create a root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Default values
    MinActionDelay = 0.5f;
    MaxActionDelay = 1.5f;
    bIsProcessingTurn = false;
    PlayerNumber = 1; // AI is player 1
    UnitColor = EUnitColor::RED;
    CurrentAction = ESAIAction::NONE;
    SelectedUnit = nullptr;
}

// Called when the game starts or when spawned
void ATBS_SmartAI::BeginPlay()
{
    Super::BeginPlay();

    // Get the game instance
    GameInstance = Cast<UTBS_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    // Find the grid
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        Grid = Cast<AGrid>(FoundActors[0]);
    }
}

// Called every frame
void ATBS_SmartAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATBS_SmartAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATBS_SmartAI::OnTurn_Implementation()
{
    // Check turn
    ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
    if (!GameMode)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Smart AI: GameMode is null"));
        return;
    }

    // Early exit if it's not AI turn
    if (GameMode->CurrentPlayer != PlayerNumber)
    {
        bIsProcessingTurn = false;
        return;
    }

    // Only here if it's the AI's turn
    bIsProcessingTurn = true;

    if (GameInstance)
    {
        GameInstance->SetTurnMessage(TEXT("Smart AI Turn"));
    }

    // Add slight delay before taking action
    bIsProcessingTurn = true;
    CurrentAction = ESAIAction::NONE;

    // Find units at the start of turn
    FindMyUnits();
    FindEnemyUnits();

    float Delay = FMath::RandRange(MinActionDelay, MaxActionDelay);
    GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &ATBS_SmartAI::ProcessTurnAction, Delay, false);
}

void ATBS_SmartAI::SetTurnState_Implementation(bool bNewTurnState)
{
    // Safely set the turn state
    bIsProcessingTurn = bNewTurnState;

    // UI update
    if (bNewTurnState)
    {
        ITBS_PlayerInterface::Execute_UpdateUI(this);
    }
}

void ATBS_SmartAI::UpdateUI_Implementation()
{
    // Update any UI elements related to AI
    GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance && bIsProcessingTurn)
    {
        FString Message = TEXT("Smart AI's Turn");

        // If there's a winner, append it to the message
        if (GameInstance->HasWinner())
        {
            Message = Message + TEXT(" | ") + GameInstance->GetWinnerMessage();
        }

        GameInstance->SetTurnMessage(Message);
    }
}

void ATBS_SmartAI::OnWin_Implementation()
{
    if (GameInstance)
    {
        // Set the winner to the AI player (index 1)
        GameInstance->SetWinner(1);

        GameInstance->SetTurnMessage(TEXT("Smart AI Wins!"));
        GameInstance->RecordGameResult(false); // Human didn't win
    }
}

void ATBS_SmartAI::OnLose_Implementation()
{
    if (GameInstance)
    {
        GameInstance->SetTurnMessage(TEXT("Smart AI Loses!"));
    }
}

void ATBS_SmartAI::OnPlacement_Implementation()
{
    if (GameInstance)
    {
        GameInstance->SetTurnMessage(TEXT("Smart AI Placing Units"));
    }

    // Add slight delay before placing units
    bIsProcessingTurn = true;
    CurrentAction = ESAIAction::PLACEMENT;
    float Delay = FMath::RandRange(MinActionDelay, MaxActionDelay);
    GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &ATBS_SmartAI::ProcessPlacementAction, Delay, false);
}

void ATBS_SmartAI::FindMyUnits()
{
    MyUnits.Empty();

    // Find all units of this player
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(Actor);
        if (Unit && Unit->GetOwnerID() == PlayerNumber && !Unit->IsDead())
        {
            MyUnits.Add(Unit);
        }
    }
}

void ATBS_SmartAI::FindEnemyUnits()
{
    EnemyUnits.Empty();

    // Find all units of the opponent
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(Actor);
        if (Unit && Unit->GetOwnerID() != PlayerNumber && !Unit->IsDead())
        {
            EnemyUnits.Add(Unit);
        }
    }
}

// Same placement logic as NaiveAI, focus on gameplay strategy
void ATBS_SmartAI::ProcessPlacementAction()
{
    // Get the game mode
    ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
    if (!GameMode)
    {
        return;
    }

    // Verify it's AI's turn
    if (GameMode->CurrentPlayer != PlayerNumber)
    {
        return;
    }

    // Track available unit types
    TArray<EUnitType> AvailableTypes;

    if (!GameMode->BrawlerPlaced[PlayerNumber])
    {
        AvailableTypes.Add(EUnitType::BRAWLER);
    }

    if (!GameMode->SniperPlaced[PlayerNumber])
    {
        AvailableTypes.Add(EUnitType::SNIPER);
    }

    // If no unit types are available, end processing
    if (AvailableTypes.Num() == 0)
    {
        CurrentAction = ESAIAction::NONE;
        bIsProcessingTurn = false;
        return;
    }

    // Try to place a unit with multiple attempts -> to avoid getting stuck in waiting
    int32 MaxPlacementAttempts = 5;
    bool Success = false;

    for (int32 Attempt = 0; Attempt < MaxPlacementAttempts && !Success; Attempt++)
    {
        // Randomly select from available unit types
        int32 RandomIndex = FMath::RandRange(0, AvailableTypes.Num() - 1);
        EUnitType TypeToPlace = AvailableTypes[RandomIndex];

        // Find random empty tile to place the unit
        int32 GridX, GridY;
        if (PickRandomTileForPlacement(GridX, GridY))
        {
            // Try to place the unit
            Success = GameMode->PlaceUnit(TypeToPlace, GridX, GridY, PlayerNumber);

            if (Success)
            {
                // Record move in game instance
                FString UnitTypeString = (TypeToPlace == EUnitType::BRAWLER) ? "Brawler" : "Sniper";
                if (GameInstance)
                {
                    GameInstance->AddMoveToHistory(PlayerNumber, UnitTypeString, "Place",
                        FVector2D::ZeroVector, FVector2D(GridX, GridY), 0);
                }

                break;
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                    FString::Printf(TEXT("Smart AI Placement - Failed to place unit at (%d,%d), trying again..."),
                        GridX, GridY));
            }
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Smart AI Placement - Could not find empty tile for attempt"));
        }
    }

    if (!Success)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Smart AI Placement - Failed after multiple attempts! Emergency handling..."));

        // Find any valid tile on the grid
        for (ATile* Tile : Grid->TileArray)
        {
            if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY && Tile->GetOwner() != -2)
            {
                FVector2D GridPos = Tile->GetGridPosition();
                int32 GridX = GridPos.X;
                int32 GridY = GridPos.Y;

                // Try placing with the first available unit type
                EUnitType TypeToPlace = AvailableTypes[0];
                Success = GameMode->PlaceUnit(TypeToPlace, GridX, GridY, PlayerNumber);

                if (Success)
                {
                    FString UnitTypeString = (TypeToPlace == EUnitType::BRAWLER) ? "Brawler" : "Sniper";
                    if (GameInstance)
                    {
                        GameInstance->AddMoveToHistory(PlayerNumber, UnitTypeString, "Place",
                            FVector2D::ZeroVector, FVector2D(GridX, GridY), 0);
                    }

                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                        TEXT("Smart AI Placement - Emergency placement successful!"));
                    break;
                }
            }
        }
    }

    // Even if couldn't place a unit -> ensure the game continues
    if (!Success)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("WARNING: Smart AI could not place unit! Marking as placed to avoid game freeze."));

        // Mark a unit as placed to prevent game freeze (choose the first available type)
        if (AvailableTypes.Contains(EUnitType::BRAWLER))
            GameMode->BrawlerPlaced[PlayerNumber] = true;
        else if (AvailableTypes.Contains(EUnitType::SNIPER))
            GameMode->SniperPlaced[PlayerNumber] = true;

        // Increment units placed to ensure game progresses
        GameMode->UnitsPlaced++;
    }

    // Reset action state
    CurrentAction = ESAIAction::NONE;
    bIsProcessingTurn = false;
}

bool ATBS_SmartAI::PickRandomTileForPlacement(int32& OutX, int32& OutY)
{
    if (!Grid)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Smart AI - Grid is null in PickRandomTileForPlacement"));
        return false;
    }

    // Create a list of all empty tiles with additional verification
    TArray<ATile*> EmptyTiles;
    int32 totalTiles = 0;
    int32 occupiedTiles = 0;
    int32 obstacleTiles = 0;
    int32 emptyTiles = 0;

    for (ATile* Tile : Grid->TileArray)
    {
        totalTiles++;

        if (!Tile)
            continue;

        ETileStatus status = Tile->GetTileStatus();
        int32 owner = Tile->GetOwner();

        if (status == ETileStatus::EMPTY)
        {
            // Double-check it's truly empty
            if (!Tile->GetOccupyingUnit() && owner != -2) // -2 is for obstacles
            {
                EmptyTiles.Add(Tile);
                emptyTiles++;
            }
            else if (owner == -2)
            {
                obstacleTiles++;
            }
        }
        else
        {
            occupiedTiles++;
        }
    }

    // Pick random empty tile
    if (EmptyTiles.Num() > 0)
    {
        int32 MaxAttempts = 10; // Prevent infinite loop
        for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
        {
            int32 RandomIndex = FMath::RandRange(0, EmptyTiles.Num() - 1);
            ATile* SelectedTile = EmptyTiles[RandomIndex];

            // Final verification that tile is truly empty
            if (SelectedTile && SelectedTile->GetTileStatus() == ETileStatus::EMPTY &&
                !SelectedTile->GetOccupyingUnit() && SelectedTile->GetOwner() != -2)
            {
                FVector2D GridPos = SelectedTile->GetGridPosition();
                OutX = GridPos.X;
                OutY = GridPos.Y;

                return true;
            }
            else
            {
                // Remove this tile from consideration and try again
                EmptyTiles.RemoveAt(RandomIndex);

                if (EmptyTiles.Num() == 0)
                    break;
            }
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Smart AI - Could not find any suitable empty tiles after verification"));
    return false;
}

void ATBS_SmartAI::ProcessTurnAction()
{
    // Find all units owned by this AI
    FindMyUnits();
    FindEnemyUnits();

    // If no units, end turn
    if (MyUnits.Num() == 0)
    {
        FinishTurn();
        return;
    }

    // Process each unit with strategic thinking
    for (AUnit* Unit : MyUnits)
    {
        if (Unit && !Unit->IsDead())
        {
            // Process the unit's action with strategic thinking
            ProcessUnitAction(Unit);

            // Add a small delay between units
            float Delay = FMath::RandRange(0.2f, 0.5f);
            FPlatformProcess::Sleep(Delay);
        }
    }

    // End the turn after processing all units
    FinishTurn();
}

void ATBS_SmartAI::ProcessUnitAction(AUnit* Unit)
{
    if (!Unit || Unit->IsDead())
        return;

    // First, check if enemies are in attack range without moving
    bool HasAttacked = false;
    if (!Unit->HasAttacked())
    {
        HasAttacked = TryAttackWithUnit(Unit);
    }

    // If we didn't attack or can still move, consider movement
    bool HasMoved = false;
    if (!Unit->HasMoved() && (!HasAttacked || Unit->GetUnitType() == EUnitType::BRAWLER))
    {
        HasMoved = TryMoveUnit(Unit);
    }

    // If we moved but didn't attack yet, try to attack now
    if (HasMoved && !HasAttacked && !Unit->HasAttacked())
    {
        TryAttackWithUnit(Unit);
    }

    // If the unit cannot move or attack, skip its turn
    if (!Unit->HasMoved() && !Unit->HasAttacked())
    {
        Unit->bHasMoved = true;
        Unit->bHasAttacked = true;

        // Record skipped turn
        if (GameInstance)
        {
            GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Skip", FVector2D::ZeroVector, FVector2D::ZeroVector, 0);
        }
    }
}

bool ATBS_SmartAI::TryMoveUnit(AUnit* Unit)
{
    if (!Unit || Unit->HasMoved())
        return false;

    // Get possible movement tiles
    TArray<ATile*> MovementTiles = Unit->GetMovementTiles();

    // If no tiles to move to, return false
    if (MovementTiles.Num() == 0)
        return false;

    // Use strategic thinking to select the best destination
    ATile* TargetTile = SelectBestMovementDestination(Unit);

    if (!TargetTile)
    {
        // If no strategic tile found, pick a random one as fallback
        int32 RandomIndex = FMath::RandRange(0, MovementTiles.Num() - 1);
        TargetTile = MovementTiles[RandomIndex];
    }

    // Record the initial position
    FVector2D FromPosition = Unit->GetCurrentTile()->GetGridPosition();
    FVector2D ToPosition = TargetTile->GetGridPosition();

    // Move the unit
    bool Success = Unit->MoveToTile(TargetTile);

    if (Success)
    {
        // Record move
        if (GameInstance)
        {
            GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Move", FromPosition, ToPosition, 0);
        }

        // Notify using game mode
        ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Move", FromPosition, ToPosition, 0);
        }
    }

    return Success;
}

bool ATBS_SmartAI::TryAttackWithUnit(AUnit* Unit)
{
    if (!Unit || Unit->HasAttacked())
        return false;

    // Get possible attack tiles
    TArray<ATile*> AttackTiles = Unit->GetAttackTiles();

    // If no tiles to attack, return false
    if (AttackTiles.Num() == 0)
        return false;

    // Select the best target to attack using strategic thinking
    AUnit* TargetUnit = SelectBestAttackTarget(Unit, AttackTiles);

    if (!TargetUnit)
        return false;

    ATile* TargetTile = TargetUnit->GetCurrentTile();
    if (!TargetTile)
        return false;

    // Get positions for recording
    FVector2D FromPosition = Unit->GetCurrentTile()->GetGridPosition();
    FVector2D ToPosition = TargetTile->GetGridPosition();

    // Attack the unit
    int32 Damage = Unit->Attack(TargetUnit);

    // Record attack
    if (GameInstance)
    {
        GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Attack", FromPosition, ToPosition, Damage);
    }

    // Notify using game mode
    ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Attack", FromPosition, ToPosition, Damage);
    }

    return true;
}

AUnit* ATBS_SmartAI::SelectBestAttackTarget(AUnit* AttackingUnit, const TArray<ATile*>& AttackableTiles)
{
    if (!AttackingUnit || AttackableTiles.Num() == 0)
        return nullptr;

    AUnit* BestTarget = nullptr;
    float BestScore = -FLT_MAX;

    for (ATile* Tile : AttackableTiles)
    {
        AUnit* TargetUnit = Tile->GetOccupyingUnit();

        // Skip if no unit on tile or it's player's unit
        if (!TargetUnit || TargetUnit->GetOwnerID() == PlayerNumber)
            continue;

        float Score = 0.0f;

        // Prioritize low health units - they're easier to eliminate
        float HealthPercentage = static_cast<float>(TargetUnit->GetUnitHealth()) /
            static_cast<float>(TargetUnit->GetMaxHealth());
        Score += (1.0f - HealthPercentage) * 100.0f;

        // Prioritize higher-value unit types
        if (TargetUnit->GetUnitType() == EUnitType::SNIPER)
            Score += 50.0f;
        else
            Score += 25.0f;

        // Consider if we can eliminate the unit (major strategic advantage)
        if (AttackingUnit->GetAverageAttackDamage() >= TargetUnit->GetUnitHealth())
            Score += 200.0f;

        // Update best target if score is higher
        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = TargetUnit;
        }
    }

    return BestTarget;
}

void ATBS_SmartAI::FinishTurn()
{
    // End our turn via the GameMode
    ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->EndTurn();
    }

    bIsProcessingTurn = false;
}

void ATBS_SmartAI::SkipUnitTurn()
{
    if (SelectedUnit)
    {
        // Mark the unit as having moved and attacked
        SelectedUnit->bHasMoved = true;
        SelectedUnit->bHasAttacked = true;

        // Record skipped turn in move history
        if (GameInstance)
        {
            GameInstance->AddMoveToHistory(PlayerNumber, SelectedUnit->GetUnitName(), "Skip", FVector2D::ZeroVector, FVector2D::ZeroVector, 0);
        }

        // Clear the selected unit
        SelectedUnit = nullptr;
    }
}

void ATBS_SmartAI::ResetActionState()
{
    CurrentAction = ESAIAction::NONE;
}

void ATBS_SmartAI::EndTurn()
{
    // End our turn via the GameMode 
    ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->EndTurn();
    }

    // Reset turn-related states
    bIsProcessingTurn = false;
    CurrentAction = ESAIAction::NONE;
    SelectedUnit = nullptr;
}

ATile* ATBS_SmartAI::SelectBestMovementDestination(AUnit* Unit)
{
    if (!Unit || EnemyUnits.Num() == 0)
        return nullptr;

    ATile* BestTile = nullptr;
    float BestScore = -FLT_MAX;
    TArray<ATile*> MovementTiles = Unit->GetMovementTiles();

    // Skip if no movement tiles available
    if (MovementTiles.Num() == 0)
        return nullptr;

    // Strategy depends on unit type
    if (Unit->GetUnitType() == EUnitType::SNIPER)
    {
        // For Snipers, maintain distance but stay in range
        for (ATile* Tile : MovementTiles)
        {
            float Score = 0;
            FVector2D TilePos = Tile->GetGridPosition();

            // Check if moving here would let it attack an enemy right after
            bool CanAttackAfterMove = false;
            bool WouldBeUnderAttack = false;

            // Simulate moving to this tile
            ATile* OriginalTile = Unit->GetCurrentTile();
            TArray<ATile*> AttackTilesAfterMove;

            // Theoretically, what would be it's attack tiles if we moved here?
            for (AUnit* Enemy : EnemyUnits)
            {
                if (!Enemy || Enemy->IsDead())
                    continue;

                ATile* EnemyTile = Enemy->GetCurrentTile();
                if (!EnemyTile)
                    continue;

                FVector2D EnemyPos = EnemyTile->GetGridPosition();

                // Simple distance calculation (Manhattan distance)
                float Distance = FMath::Abs(TilePos.X - EnemyPos.X) + FMath::Abs(TilePos.Y - EnemyPos.Y);

                // Check if enemy would be in attack range
                if (Unit->GetAttackRange() >= Distance)
                {
                    CanAttackAfterMove = true;
                    Score += 100.0f; // Strong bonus for being able to attack
                }

                // Check if unit'd be in enemy attack range
                if (Enemy->GetAttackRange() >= Distance)
                {
                    WouldBeUnderAttack = true;
                    Score -= 50.0f; // Penalty for being under attack
                }

                // For snipers, prefer medium distance - not too close, not too far
                float OptimalDistance = Unit->GetAttackRange() * 0.7f;
                float DistanceScore = 40.0f - FMath::Abs(Distance - OptimalDistance) * 5.0f;
                Score += DistanceScore;
            }

            // Compare and update best tile
            if (Score > BestScore)
            {
                BestScore = Score;
                BestTile = Tile;
            }
        }
    }
    else // Brawler
    {
        // For Brawlers, aggresively approach enemies
        for (ATile* Tile : MovementTiles)
        {
            float Score = 0;
            FVector2D TilePos = Tile->GetGridPosition();

            // Find closest enemy
            float MinDistance = FLT_MAX;
            AUnit* ClosestEnemy = nullptr;

            for (AUnit* Enemy : EnemyUnits)
            {
                if (!Enemy || Enemy->IsDead())
                    continue;

                ATile* EnemyTile = Enemy->GetCurrentTile();
                if (!EnemyTile)
                    continue;

                FVector2D EnemyPos = EnemyTile->GetGridPosition();
                float Distance = FMath::Abs(TilePos.X - EnemyPos.X) + FMath::Abs(TilePos.Y - EnemyPos.Y);

                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    ClosestEnemy = Enemy;
                }

                // Bonus for getting in attack range
                if (Distance <= Unit->GetAttackRange())
                {
                    Score += 100.0f;
                }
            }

            if (ClosestEnemy)
            {
                // Brawlers want to get close - the closer the better
                Score += 100.0f - (MinDistance * 10.0f);

                // Extra weight for enemies with low health
                if (ClosestEnemy->GetUnitHealth() < ClosestEnemy->GetMaxHealth() * 0.5f)
                {
                    Score += 50.0f;
                }
            }

            // Compare and update best tile
            if (Score > BestScore)
            {
                BestScore = Score;
                BestTile = Tile;
            }
        }
    }

    return BestTile;
}