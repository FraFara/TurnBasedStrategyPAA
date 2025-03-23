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

    //// Enable proper collision for the static mesh component
    //StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    //StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    //StaticMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    //StaticMeshComponent->SetGenerateOverlapEvents(true);

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

    // Debug message to confirm obstacle setting
    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
        FString::Printf(TEXT("SET OBSTACLE at (%f,%f) - Status: %d, Owner: %d"),
            TileGridPosition.X, TileGridPosition.Y,
            (int32)Status, PlayerOwner));
}

// Add this helper method to explicitly verify if a tile is correctly set as an obstacle
bool ATile::VerifyObstacleStatus() const
{
    bool isObstacle = (Status == ETileStatus::OCCUPIED && PlayerOwner == -2);

    // Debug verification result
    if (isObstacle)
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green,
            FString::Printf(TEXT("Verified obstacle at (%f,%f)"),
                TileGridPosition.X, TileGridPosition.Y));
    }
    else if (PlayerOwner == -2)
    {
        // Something is wrong - has owner -2 but wrong status
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
            FString::Printf(TEXT("INVALID OBSTACLE at (%f,%f) - Status: %d, Owner: %d"),
                TileGridPosition.X, TileGridPosition.Y,
                (int32)Status, PlayerOwner));
    }

    return isObstacle;
}

void ATile::SetHighlightForMovement()
{
    // NEVER highlight obstacle tiles - check explicitly
    if (Status == ETileStatus::OCCUPIED && PlayerOwner == -2)
    {
        // Debug that this obstacle is being skipped
        GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red,
            FString::Printf(TEXT("Skipping obstacle highlight at (%f,%f)"),
                TileGridPosition.X, TileGridPosition.Y));
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
    // NEVER highlight obstacle tiles - check explicitly
    if (Status == ETileStatus::OCCUPIED && PlayerOwner == -2)
    {
        // Debug that this obstacle is being skipped
        GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red,
            FString::Printf(TEXT("Skipping obstacle attack highlight at (%f,%f)"),
                TileGridPosition.X, TileGridPosition.Y));
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
    // A tile is an obstacle if it has OCCUPIED status and owner ID of -2
    return (Status == ETileStatus::OCCUPIED && PlayerOwner == -2);
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

    //// Adjust collision bounds if needed
    //if (StaticMeshComponent)
    //{
    //    // Get the bounds of the static mesh
    //    FBoxSphereBounds Bounds = StaticMeshComponent->CalcBounds(StaticMeshComponent->GetComponentTransform());

    //    // Create a box component with these bounds
    //    UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this);
    //    BoxComponent->SetupAttachment(StaticMeshComponent);
    //    BoxComponent->SetBoxExtent(Bounds.BoxExtent);
    //    BoxComponent->SetRelativeLocation(Bounds.Origin);
    //    BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    //    BoxComponent->SetCollisionResponseToAllChannels(ECR_Block);
    //}

    // Fix for inconsistent obstacle states - owner -2 should always be OCCUPIED status
    if (PlayerOwner == -2)
    {
        if (Status != ETileStatus::OCCUPIED)
        {
            // Correct the status
            Status = ETileStatus::OCCUPIED;
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                FString::Printf(TEXT("Fixed inconsistent obstacle at (%f,%f)"),
                    TileGridPosition.X, TileGridPosition.Y));
        }

        // Ensure obstacle has its proper visual appearance
        if (UFunction* Function = FindFunction(TEXT("SetObstacleMaterial")))
        {
            ProcessEvent(Function, nullptr);
        }
    }

    // Check for reverse inconsistency - OCCUPIED status but not owner -2
    if (Status == ETileStatus::OCCUPIED && PlayerOwner != -2 && !OccupyingUnit)
    {
        // This might be a phantom obstacle - log it
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            FString::Printf(TEXT("Warning: Occupied tile with no unit at (%f,%f), Owner: %d"),
                TileGridPosition.X, TileGridPosition.Y, PlayerOwner));
    }
}

// Called every frame
//void ATile::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}