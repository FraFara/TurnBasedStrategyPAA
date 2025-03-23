// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Grid.h"
#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AUnit::AUnit()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Creates scne and mesh subobjects
    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

    SetRootComponent(SceneComponent);
    StaticMeshComponent->SetupAttachment(SceneComponent);

    // Set proper collision settings for unit selection
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    StaticMeshComponent->SetCollisionObjectType(ECC_Pawn);
    StaticMeshComponent->SetGenerateOverlapEvents(true);

    // Make sure the mesh is visible and can be clicked
    StaticMeshComponent->SetVisibility(true);
    StaticMeshComponent->bSelectable = true;

    // Initialize variables
    AttackType = EAttackType::NONE;
    UnitType = EUnitType::NONE;

    bHasMoved = false;
    bHasAttacked = false;
    CurrentTile = nullptr;

}

void AUnit::UpdateAppearanceByTeam()
{
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
    Super::BeginPlay();

    // Finds the grid in the world
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        Grid = Cast<AGrid>(FoundActors[0]);
    }

}

// Called every frame
void AUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AUnit::SetOwnerID(int32 InOwnerID)
{
    OwnerID = InOwnerID;

    // Update visual appearance based on team
    UpdateAppearanceByTeam();
}

FString AUnit::GetUnitName() const
{
    return UnitName;
}

int32 AUnit::GetOwnerID() const
{
    return OwnerID;
}

int32 AUnit::GetUnitHealth() const
{
    return Health;
}

int32 AUnit::GetMaxHealth() const
{
    return MaxHealth;
}

EUnitType AUnit::GetUnitType() const
{
    return UnitType;
}

EAttackType AUnit::GetAttackType() const
{
    return AttackType;
}

int32 AUnit::GetAttackRange() const
{
    return AttackRange;
}

int32 AUnit::GetMinDamage() const
{
    return MinDamage;
}

int32 AUnit::GetMaxDamage() const
{
    return MaxDamage;
}

float AUnit::GetAverageAttackDamage() const
{
    return ((static_cast<float>(MinDamage) + static_cast<float>(MaxDamage)) / 2.f);
}

FString AUnit::GetLiveHealth()
{
    return FString::Printf(TEXT("Hp: %d / %d"), Health, MaxHealth);
}

float AUnit::GethealthPercentage()
{
    float HealthPercentage = (static_cast<float>(Health) / MaxHealth);

    return HealthPercentage;
}


// Changes the tile status and ownership
void AUnit::InitializePosition(ATile* Tile)
{
    if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY)
    {
        // Debug message
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("InitializePosition called for %sOwner: %d"), *GetUnitName(), OwnerID));

        CurrentTile = Tile;

        // Update the tile status
        Tile->SetTileStatus(OwnerID, ETileStatus::OCCUPIED);

        // Move the unit to the tile's location
        SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 20.0f));

        // Tells the tile this unit is occupying it
        Tile->SetOccupyingUnit(this);
    }
    else
    {
        // Debug message for failure
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("InitializePosition failed - Tile is null or not empty")));
    }
}

// If the unit can move updates the tiles ownerships
bool AUnit::MoveToTile(ATile* Tile)
{
    if (!Tile || bHasMoved) // Checks the possibility to move
        return false;

    TArray<ATile*> ValidTiles = GetMovementTiles();
    if (!ValidTiles.Contains(Tile))
        return false;

    // Updates the old tile
    CurrentTile->SetTileStatus(AGrid::NOT_ASSIGNED, ETileStatus::EMPTY);
    CurrentTile->SetOccupyingUnit(nullptr); // Clear the old tile's occupying unit

    // Update the new tile
    CurrentTile = Tile;
    Tile->SetTileStatus(OwnerID, ETileStatus::OCCUPIED);
    Tile->SetOccupyingUnit(this); // Set the new tile's occupying unit

    // Move the unit to the tile's location
    SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 20.0f)); // Offset slightly above the tile to avoid assets compenetrations (as suggested in class)

    bHasMoved = true; // Sets HasMoved to true to avoid multiple movement actions
    return true;
}

// Checks possible tiles to occupy
TArray<ATile*> AUnit::GetMovementTiles()
{
    TArray<ATile*> ValidTiles; // Array to store results

    if (!CurrentTile || !Grid) // Checks if the unit has a current tile and if the grid exists
        return ValidTiles;

    FVector2D CurrentPos = CurrentTile->GetGridPosition();

    // Algorithm BFS to find valid movement tiles
    TArray<ATile*> Queue;
    TMap<ATile*, int32> Distance;

    Queue.Add(CurrentTile); // Adds tile to queue and sets distance to 0
    Distance.Add(CurrentTile, 0);

    while (Queue.Num() > 0) //While Queue is not empty
    {
        ATile* Tile = Queue[0]; // analize and remove the first item from the Queue
        Queue.RemoveAt(0);

        FVector2D TilePos = Tile->GetGridPosition(); // Gets the distance of a tile from the starting tile
        int32 CurrentDistance = Distance[Tile];

        if (CurrentDistance < MovementRange) // If still in range
        {
            // Checks adjacent tiles (HASNEXT)
            TArray<FVector2D> Directions = {
                FVector2D(0, 1),
                FVector2D(0, -1),
                FVector2D(1, 0),
                FVector2D(-1, 0)
            };

            // For every adjacent tile...
            for (FVector2D& Dir : Directions)
            {
                FVector2D NewPos = TilePos + Dir;

                // ...checks if the new position is valid
                if (Grid->TileMap.Contains(NewPos))
                {
                    ATile* NextTile = Grid->TileMap[NewPos];

                    // Only add tile if it's empty AND not an obstacle (owner != -2)
                    if (NextTile->GetTileStatus() == ETileStatus::EMPTY &&
                        NextTile->GetOwner() != -2 &&  // Check it's not an obstacle
                        !Distance.Contains(NextTile))
                    {
                        Queue.Add(NextTile);
                        Distance.Add(NextTile, CurrentDistance + 1);
                    }
                }
            }
        }
    }

    // Add all tiles except current
    for (auto& Elem : Distance)
    {
        if (Elem.Key != CurrentTile)
        {
            ValidTiles.Add(Elem.Key);
        }
    }

    return ValidTiles;
}

// Checks attackable tiles
TArray<ATile*> AUnit::GetAttackTiles() // Similar structure as the GetMovementTiles
{
    TArray<ATile*> ValidTiles;

    if (!CurrentTile || !Grid)
        return ValidTiles;

    // Algorithm BFS to find valid attack tiles
    TArray<ATile*> Queue;
    TMap<ATile*, int32> Distance;

    Queue.Add(CurrentTile);
    Distance.Add(CurrentTile, 0);

    while (Queue.Num() > 0)
    {
        ATile* Tile = Queue[0];
        Queue.RemoveAt(0);

        FVector2D TilePos = Tile->GetGridPosition();
        int32 CurrentDistance = Distance[Tile];

        // Checks if this is an attackable tile (if it is occupied by enemy)
        if (Tile->GetTileStatus() == ETileStatus::OCCUPIED && Tile->GetOwner() != OwnerID)
        {
            ValidTiles.Add(Tile);
        }

        // If still within attack range
        if (CurrentDistance < AttackRange)
        {
            // Checks adjacent tiles (HASNEXT)
            TArray<FVector2D> Directions = {
                FVector2D(0, 1),
                FVector2D(0, -1),
                FVector2D(1, 0),
                FVector2D(-1, 0)
            };

            // For every adjacent tile...
            for (const FVector2D& Dir : Directions)     //const?
            {
                FVector2D NewPos = TilePos + Dir;

                // ...checks if the new position is valid
                if (Grid->TileMap.Contains(NewPos))
                {
                    ATile* NextTile = Grid->TileMap[NewPos];

                    // If the tile hasn't been visited yet
                    if (!Distance.Contains(NextTile))
                    {
                        Queue.Add(NextTile);
                        Distance.Add(NextTile, CurrentDistance + 1);
                    }
                }
            }
        }
    }

    return ValidTiles;
}

// Attack action
int32 AUnit::Attack(AUnit* TargetUnit)
{
    if (!TargetUnit || bHasAttacked)
        return 0;

    // Checks if the target is within attack range
    TArray<ATile*> ValidAttackTiles = GetAttackTiles();
    if (!ValidAttackTiles.Contains(TargetUnit->GetCurrentTile()))
        return 0;

    // Calculates damage (random between min and max)
    int32 Damage = FMath::RandRange(MinDamage, MaxDamage);

    // Applies damage to target
    TargetUnit->ReceiveDamage(Damage);

    bHasAttacked = true;  // Sets HasAttacked to true to avoid multiple attack actions
    return Damage;
}

// Reduces health based on damage received
void AUnit::ReceiveDamage(int32 DamageAmount)
{
    Health -= DamageAmount; // Reduces health keeping it within the limits
    Health = FMath::Max(0, FMath::Min(Health, MaxHealth));

    // If health reaches 0, unit dies
    if (Health <= 0)
    {
        // Get the game mode and notify it that a unit was destroyed
        ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            // Notify the game mode about which player's unit was destroyed
            GameMode->NotifyUnitDestroyed(OwnerID);
        }

        CurrentTile->SetTileStatus(AGrid::NOT_ASSIGNED, ETileStatus::EMPTY);
        CurrentTile->SetOccupyingUnit(nullptr); // Clear the tile's unit reference

        Destroy();
    }
}

// Boolean for unit's death
bool AUnit::IsDead() const
{
    return Health <= 0;
}

// Resets the turn, resetting the movement and attack action of the unit
void AUnit::ResetTurn()
{
    bHasMoved = false;
    bHasAttacked = false;
}

// Returns occupying tile
ATile* AUnit::GetCurrentTile() const
{
    return CurrentTile;
}

// Track unit's movement and attack actions
bool AUnit::HasMoved() const
{
    return bHasMoved;
}

bool AUnit::HasAttacked() const
{
    return bHasAttacked;
}