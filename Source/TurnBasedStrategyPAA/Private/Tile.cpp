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

    // Enable proper collision for the static mesh component
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    StaticMeshComponent->SetCollisionObjectType(ECC_WorldStatic);

    // Make sure the mesh is visible and can be clicked
    StaticMeshComponent->SetVisibility(true);
    StaticMeshComponent->bSelectable = true;

       Status = ETileStatus::EMPTY;
    PlayerOwner = -1;
    TileGridPosition = FVector2D(0, 0);
    
    // Initialize the new variables
    OriginalMaterial = nullptr;
    bIsHighlighted = false;
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
    // Set the tile status
    SetTileStatus(-2, ETileStatus::OCCUPIED);

    // The actual visual changes will be handled in Blueprint
}

void ATile::SetHighlight(bool bHighlighted, UMaterialInterface* HighlightMaterial)
{
    // Store the original material the first time we highlight
    if (!OriginalMaterial && StaticMeshComponent)
    {
        OriginalMaterial = StaticMeshComponent->GetMaterial(0);
    }

    // Apply the highlight material
    if (StaticMeshComponent && HighlightMaterial)
    {
        StaticMeshComponent->SetMaterial(0, HighlightMaterial);
        this->bIsHighlighted = bHighlighted; // Use 'this->' to clarify we're using the class member
    }
}

void ATile::ClearHighlight()
{
    // Restore the original material
    if (StaticMeshComponent && OriginalMaterial)
    {
        StaticMeshComponent->SetMaterial(0, OriginalMaterial);
        this->bIsHighlighted = false;
    }
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
    Super::BeginPlay();

}

// Called every frame
//void ATile::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}