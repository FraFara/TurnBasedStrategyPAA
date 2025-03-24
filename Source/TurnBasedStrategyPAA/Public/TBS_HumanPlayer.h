// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Unit.h"
#include "TBS_PlayerInterface.h"
#include "TBS_GameInstance.h"
#include "Camera/CameraComponent.h"
#include "TBS_HumanPlayer.generated.h"

// Forward declarations
enum class EUnitColor : uint8;

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

	// Player identification properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	int32 PlayerNumber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	EUnitColor UnitColor;

	// keeps track of turn
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game State")
	bool IsMyTurn = false;

	// Current action
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
	EPlayerAction CurrentAction;

	UPROPERTY(BlueprintReadOnly, Category = "Unit Info")
	AUnit* LastClickedUnit;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Game instance reference
	UPROPERTY()
	class UTBS_GameInstance* GameInstance;

	// Array of Player's units
	UPROPERTY()
	TArray<AUnit*> MyUnits;

	// Marks the selected unit
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	AUnit* SelectedUnit;

	// Currently selected unit type for placement
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	EUnitType UnitToPlace;

	// Tile selected for unit placement
	UPROPERTY()
	ATile* CurrentPlacementTile;

	// Currently highlighted tiles
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<ATile*> HighlightedMovementTiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TArray<ATile*> HighlightedAttackTiles;

	// Highlight materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* MovementHighlightMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* AttackHighlightMaterial;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Interface implementations
	virtual void OnPlacement_Implementation() override;
	virtual void OnTurn_Implementation() override;
	virtual void OnWin_Implementation() override;
	virtual void OnLose_Implementation() override;
	virtual void SetTurnState_Implementation(bool bIsMyTurn) override;
	virtual void UpdateUI_Implementation() override;

	void FindMyUnits();

	// Called on left mouse click
	UFUNCTION()
	void OnClick();

	// Called on right mouse click
	UFUNCTION()
	void OnRightClick();

	// Select Unit For Placement
	void SelectBrawlerForPlacement();
	void SelectSniperForPlacement();

	// Spawns new unit on grid
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void PlaceUnit(int32 GridX, int32 GridY, EUnitType Type);

	UFUNCTION(BlueprintCallable, Category = "Unit Placement")
	void SetCurrentPlacementTile(ATile* Tile);

	UFUNCTION(BlueprintCallable, Category = "Unit Placement")
	ATile* GetCurrentPlacementTile();

	UFUNCTION(BlueprintCallable, Category = "Unit Placement")
	void ClearCurrentPlacementTile();

	// Skip the turn for the current unit
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SkipUnitTurn();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void ResetActionState();

	// Check if all units have completed their actions
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void CheckAllUnitsFinished();

	// End the player's turn
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void EndTurn();

	// Methods for highlighting
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void HighlightMovementTiles();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void HighlightAttackTiles();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void ClearHighlightedTiles();

	// UI Selection Detector
	UFUNCTION(BlueprintCallable, Category = "UI")
	FString GetSelectionMessage() const;
};