// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Unit.h"
#include "TBS_PlayerInterface.h"
#include "TBS_GameInstance.h"
#include "Camera/CameraComponent.h"
#include "TBS_HumanPlayer.generated.h"

// Define an enum for player actions
UENUM(BlueprintType)
enum class EPlayerAction : uint8
{
	NONE        UMETA(DisplayName = "None"),
	MOVEMENT    UMETA(DisplayName = "Movement"),
	ATTACK      UMETA(DisplayName = "Attack"),
	PLACEMENT   UMETA(DisplayName = "Unit Placement")
};

UCLASS()
class TURNBASEDSTRATEGYPAA_API ATBS_HumanPlayer : public APawn, public ITBS_PlayerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATBS_HumanPlayer();

	// camera component attacched to player pawn
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// keeps track of turn
	bool IsMyTurn = false;

	// Array of Player's units
	UPROPERTY()
	TArray<AUnit*> MyUnits;

	// Marks the selected unit
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	AUnit* SelectedUnit;

	// Currently selected unit type for placement
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	EUnitType UnitToPlace;

	// Currently highlighted tiles
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<ATile*> HighlightedMovementTiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<ATile*> HighlightedAttackTiles;

	// Current action
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	EPlayerAction CurrentAction;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnTurn() override;
	virtual void OnWin() override;
	virtual void OnLose() override;
	virtual void OnPlacement() override;
	void FindMyUnits();

	// Called on left mouse click
	UFUNCTION()
	void OnClick();

	// Called on right mouse click
	UFUNCTION()
	void OnRightClick();

	// Spawns new unit on grid
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void PlaceUnit(int32 GridX, int32 GridY, EUnitType Type);

	// Skip the turn for the current unit
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SkipUnitTurn();

	// End the player's turn
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void EndTurn();

	// Methods for highlighting
	void HighlightMovementTiles();
	void HighlightAttackTiles();
	void ClearHighlightedTiles();

	// Parameter to keep track of the camera position
	//int32 CameraPosition = 0;
	//// Method to change the camera position
	//void ChangeCameraPosition();

};
