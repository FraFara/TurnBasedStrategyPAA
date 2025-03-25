// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tile.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

// macro declaration for a dynamic multicast delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReset);

UCLASS()
class TURNBASEDSTRATEGYPAA_API AGrid : public AActor
{
	GENERATED_BODY()

public:
	// array of pointers to Tiles to keep track of them
	UPROPERTY(Transient)
	TArray<ATile*> TileArray;

	//given a position returns a tile
	UPROPERTY(Transient)
	TMap<FVector2D, ATile*> TileMap;

	//variable to specify the padding in between tiles
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NextCellPositionMultiplier;

	static const int32 NOT_ASSIGNED = -1;

	UPROPERTY(BlueprintAssignable)
	FOnReset OnResetEvent;

	// size of grid
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Size;

	// TSubclassOf template class that provides UClass type safety
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATile> TileClass;

	// tile padding percentage
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellPadding;

	// tile size
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TileSize;
	
public:	
	// Sets default values for this actor's properties
	AGrid();

	// Called when an instance of this class is placed (in editor) or spawned
	virtual void OnConstruction(const FTransform& Transform) override;


	// remove all signs from the grid
	UFUNCTION(BlueprintCallable)
	void ResetGrid();

	// generate an empty game grid
	void GenerateGrid();

	// return the array of tile pointers
	TArray<ATile*>& GetTileArray();

	// return (x,y) position given a relative position
	FVector2D GetXYPositionByRelativeLocation(const FVector& Location) const;

	void ValidateAllObstacles();

	bool ValidateConnectivity();

	void DiagnoseGridState();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// return a (x,y) position given a hit (click) on a grid tile
	FVector2D GetPosition(const FHitResult& Hit);

	// given (x,y) position returns a (x,y,z) position
	
	FVector GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const;

	// checking if is a valid field position
	inline bool IsValidPosition(const FVector2D Position) const;

//public:	
//	// Called every frame
//	virtual void Tick(float DeltaTime) override;

};
