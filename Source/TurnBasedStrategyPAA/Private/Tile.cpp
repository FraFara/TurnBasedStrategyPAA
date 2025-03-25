// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile.h"

// Sets default values
ATile::ATile()
{

    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    // template function that creates a components
    Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

    // every actor has a RootComponent that defines the transform in the World
    SetRootComponent(Scene);
    StaticMeshComponent->SetupAttachment(Scene);

    // Set the StaticMeshComponent to be at the origin of the Scene component
    StaticMeshComponent->SetRelativeLocation(FVector::ZeroVector);

    // Make sure the mesh is visible and can be clicked
    StaticMeshComponent->SetVisibility(true);
    StaticMeshComponent->bSelectable = true;

    Status = ETileStatus::EMPTY;
    PlayerOwner = -1;
    TileGridPosition = FVector2D(0, 0);

    OriginalMaterial = nullptr;
    bIsHighlighted = false;
    OccupyingUnit = nullptr;
}

void ATile::SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus)
{
    PlayerOwner = TileOwner;
    Status = TileStatus;
}

ETileStatus ATile::GetTileStatus()
{
    return Status;
}

int32 ATile::GetOwner()
{
    return PlayerOwner;
}

void ATile::SetGridPosition(const double InX, const double InY)
{
    TileGridPosition.Set(InX, InY);
}

FVector2D ATile::GetGridPosition()
{
    return TileGridPosition;
}

void ATile::SetOccupyingUnit(AUnit* Unit)
{
    OccupyingUnit = Unit;
}

AUnit* ATile::GetOccupyingUnit()
{
    return OccupyingUnit;
}

void ATile::SetAsObstacle()
{
    // Set the tile status - owner -2 indicates obstacle
    SetTileStatus(-2, ETileStatus::OCCUPIED);

    // Ensure no unit is occupying this tile
    if (OccupyingUnit)
    {
        OccupyingUnit = nullptr;
    }

    // Call the blueprint event to update the visual appearance
    if (UFunction* Function = FindFunction(TEXT("SetObstacleMaterial")))
    {
        ProcessEvent(Function, nullptr);
    }
}

// Add this helper method to explicitly verify if a tile is correctly set as an obstacle
bool ATile::VerifyObstacleStatus() const
{
    bool isObstacle = (Status == ETileStatus::OCCUPIED && PlayerOwner == -2);

    return isObstacle;
}

void ATile::SetHighlightForMovement()
{
    // Never highlight obstacle tiles
    if (IsObstacle() || GetOwner() == -2 ||
        (GetTileStatus() == ETileStatus::OCCUPIED && !GetOccupyingUnit()))
    {
        return;
    }

    // Store the original material the first time we highlight
    if (!OriginalMaterial && StaticMeshComponent)
    {
        OriginalMaterial = StaticMeshComponent->GetMaterial(0);
    }

    // Call the blueprint event to update the visual appearance
    if (UFunction* Function = FindFunction(TEXT("HighlightForMovement")))
    {
        ProcessEvent(Function, nullptr);
        bIsHighlighted = true;
    }
}

void ATile::SetHighlightForAttack()
{
    // Never highlight obstacle tiles
    if (IsObstacle() || GetOwner() == -2 ||
        (GetTileStatus() == ETileStatus::OCCUPIED && !GetOccupyingUnit()))
    {
        return;
    }

    // Store the original material the first time we highlight
    if (!OriginalMaterial && StaticMeshComponent)
    {
        OriginalMaterial = StaticMeshComponent->GetMaterial(0);
    }

    // Call the blueprint event to update the visual appearance
    if (UFunction* Function = FindFunction(TEXT("HighlightForAttack")))
    {
        ProcessEvent(Function, nullptr);
        bIsHighlighted = true;
    }
}

void ATile::SetHighlightForSelection()
{
    // Never highlight obstacle tiles
    if (IsObstacle() || GetOwner() == -2 ||
        (GetTileStatus() == ETileStatus::OCCUPIED && !GetOccupyingUnit()))
    {
        return;
    }

    // Store the original material the first time we highlight
    if (!OriginalMaterial && StaticMeshComponent)
    {
        OriginalMaterial = StaticMeshComponent->GetMaterial(0);
    }

    // Call the blueprint event to update the visual appearance
    if (UFunction* Function = FindFunction(TEXT("HighlightForSelection")))
    {
        ProcessEvent(Function, nullptr);
        bIsHighlighted = true;
    }
}

void ATile::ClearHighlight()
{
    // If this is an obstacle tile, don't reset its material
    if (Status == ETileStatus::OCCUPIED && PlayerOwner == -2)
    {
        // Just remove the highlight state without changing the material
        bIsHighlighted = false;
        return;
    }

    // For non-obstacle tiles, restore the original material
    if (StaticMeshComponent && OriginalMaterial)
    {
        StaticMeshComponent->SetMaterial(0, OriginalMaterial);
        bIsHighlighted = false;
    }
}

bool ATile::IsObstacle() const
{
    return (Status == ETileStatus::OCCUPIED && PlayerOwner == -2) ||
        (PlayerOwner == -2) || // Even if status is wrong
        (Status == ETileStatus::OCCUPIED && !OccupyingUnit); // Occupied with no unit = obstacle
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
    Super::BeginPlay();

    // Store original material
    if (StaticMeshComponent)
    {
        OriginalMaterial = StaticMeshComponent->GetMaterial(0);
    }

    // Fix for inconsistent obstacle states
    if (PlayerOwner == -2)
    {
        if (Status != ETileStatus::OCCUPIED)
        {
            // Correct the status
            Status = ETileStatus::OCCUPIED;
        }

        // Ensure obstacle has its proper visual appearance
        if (UFunction* Function = FindFunction(TEXT("SetObstacleMaterial")))
        {
            ProcessEvent(Function, nullptr);
        }
    }
}

// Called every frame
//void ATile::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}