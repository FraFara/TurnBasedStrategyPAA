// Fill out your copyright notice in the Description page of Project Settings.


#include "Grid.h"
#include "Kismet/GameplayStatics.h"
#include "TBS_GameMode.h"

// Sets default values
AGrid::AGrid()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	//Size of the grid (25x25)
	Size = 25;
	// tile dimension
	TileSize = 100.f;
	// tile padding percentage 
	CellPadding = 0.12f;

}

void AGrid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// normalized tilepadding
	NextCellPositionMultiplier = FMath::RoundToDouble(((TileSize + TileSize * CellPadding) / TileSize) * 100) / 100;
}

// Called when the game starts or when spawned
void AGrid::BeginPlay()
{
	Super::BeginPlay();
	GenerateGrid();

}

// Resets the grid to empty
void AGrid::ResetGrid()
{
	for (ATile* Obj : TileArray) // make each tile of the grid empty
	{
		Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
	}
	// send broadcast event to registered objects 
	OnResetEvent.Broadcast();
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	GameMode->bIsGameOver = false;
	GameMode->CurrentPhase = EGamePhase::NONE;
	GameMode->SimulateCoinToss();
}

//Generates a squared (size x size) grid
void AGrid::GenerateGrid()
{
	if (!TileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("TileClass is not set! Please assign it in the Blueprint."));
		return;
	}

	for (int32 IndexX = 0; IndexX < Size; IndexX++)
	{
		for (int32 IndexY = 0; IndexY < Size; IndexY++)
		{
			FVector Location = GetRelativeLocationByXYPosition(IndexX, IndexY);

			// Check if spawning is successful
			ATile* Obj = GetWorld()->SpawnActor<ATile>(TileClass, Location, FRotator::ZeroRotator);
			if (!Obj)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to spawn tile at (%d, %d)"), IndexX, IndexY);
				continue;
			}
			const float TileScale = TileSize / 100.0f;
			const float Zscaling = 0.2f;
			Obj->SetActorScale3D(FVector(TileScale, TileScale, Zscaling));
			Obj->SetGridPosition(IndexX, IndexY);
			TileArray.Add(Obj);
			TileMap.Add(FVector2D(IndexX, IndexY), Obj);
		}
	}
}

//Clicking on a tile returns its position
FVector2D AGrid::GetPosition(const FHitResult& Hit)
{
	return Cast<ATile>(Hit.GetActor())->GetGridPosition();
}

TArray<ATile*>& AGrid::GetTileArray()
{
	return TileArray;
}

FVector AGrid::GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const
{
	return TileSize * NextCellPositionMultiplier * FVector(InX, InY, 0);
}

FVector2D AGrid::GetXYPositionByRelativeLocation(const FVector& Location) const
{
	// Debug the input location
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
		FString::Printf(TEXT("Converting world location: X=%.2f, Y=%.2f"), Location.X, Location.Y));

	// Calculate the exact grid position 
	const float XPos = Location.X / (TileSize * NextCellPositionMultiplier);
	const float YPos = Location.Y / (TileSize * NextCellPositionMultiplier);

	// Get the integer part (grid coordinate)
	const int32 GridX = FMath::FloorToInt(XPos);
	const int32 GridY = FMath::FloorToInt(YPos);

	// Calculate the fractional part (position within tile)
	const float FractX = XPos - GridX;
	const float FractY = YPos - GridY;

	// Debug the conversion
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
		FString::Printf(TEXT("Converted to grid pos: X=%d (%.2f), Y=%d (%.2f)"),
			GridX, FractX, GridY, FractY));

	return FVector2D(GridX, GridY);
}

//returns true if the tile is in the grid
inline bool AGrid::IsValidPosition(const FVector2D Position) const
{
	return 0 <= Position.X && Position.X < Size && 0 <= Position.Y && Position.Y < Size;
}

FVector AGrid::GetWorldLocationFromGrid(int32 GridX, int32 GridY)
{
	return FVector();
}

// Called every frame
//void AGrid::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}