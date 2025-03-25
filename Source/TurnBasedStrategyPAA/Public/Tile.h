// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

class AUnit;

UENUM()
enum class ETileStatus : uint8
{
	EMPTY		  UMETA(DisplayName = "Empty"),
	OCCUPIED      UMETA(DisplayName = "Occupied"),
};

UCLASS()
class TURNBASEDSTRATEGYPAA_API ATile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATile();

	// set the player owner and the status of a tile
	void SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus);

	// get the tile status
	ETileStatus GetTileStatus();

	// get the tile owner
	int32 GetOwner();

	// set the (x, y) position
	void SetGridPosition(const double InX, const double InY);

	// get the (x, y) position
	FVector2D GetGridPosition();

	// Set the occupying unit
	void SetOccupyingUnit(AUnit* Unit);

	// returns the unit occupying the tile
	UFUNCTION(BlueprintCallable, Category = "Tile")
	AUnit* GetOccupyingUnit();

	//Needed to add a mesh to the "Tile"
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UFUNCTION(BlueprintCallable, Category = "Tile")
	void SetAsObstacle();

	// Called to Highlight movement tiles
	void SetHighlightForMovement();

	// Called to Highlight attack tiles
	void SetHighlightForAttack();

	// Called to Highlight selected unit's tile
	void SetHighlightForSelection();

	void ClearHighlight();

	// Check if this tile is an obstacle
	bool IsObstacle() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// To add visuals to the scene
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Scene;

	//Status of the tile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETileStatus Status;

	//Who is actively occupying the tile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlayerOwner;

	// (x, y) position of the tile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D TileGridPosition;

	// Pointer to occupying unit
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AUnit* OccupyingUnit;

	UMaterialInterface* OriginalMaterial;
	bool bIsHighlighted;

	//public:	
	//	// Called every frame
	//	virtual void Tick(float DeltaTime) override;
	//
};