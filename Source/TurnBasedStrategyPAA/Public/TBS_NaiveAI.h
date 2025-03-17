// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TBS_PlayerInterface.h"
#include "Unit.h"
#include "Tile.h"
#include "TBS_NaiveAI.generated.h"

UCLASS()
class TURNBASEDSTRATEGYPAA_API ATBS_NaiveAI : public APawn, public ITBS_PlayerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATBS_NaiveAI();

	// Reference to game instance
	UPROPERTY()
	class UTBS_GameInstance* GameInstance;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Random delay between AI actions
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinActionDelay;

	UPROPERTY(EditAnywhere, Category = "AI")
	float MaxActionDelay;

	// Flag to track if AI is currently processing a turn
	bool bIsProcessingTurn;

	// Handle for the timer that controls the AI's thinking time
	FTimerHandle ActionTimerHandle;

	// Reference to the grid
	UPROPERTY()
	class AGrid* Grid;

	// Current unit being controlled by AI
	UPROPERTY()
	AUnit* CurrentUnit;

	// Array of AI's units
	UPROPERTY()
	TArray<AUnit*> MyUnits;

	// Current placement phase unit type
	EUnitType CurrentPlacementType;

	// Finds all units owned by this AI
	void FindMyUnits();

	// Executes a move for a single unit
	void ProcessUnitAction(AUnit* Unit);

	// Handle unit movement
	bool TryMoveUnit(AUnit* Unit);

	// Handle unit attack
	bool TryAttackWithUnit(AUnit* Unit);

	// Pick a random tile for unit placement
	bool PickRandomTileForPlacement(int32& OutX, int32& OutY);

	// AI action processing methods
	void ProcessPlacementAction();
	void ProcessTurnAction();
	void FinishTurn();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// PlayerInterface functions - same as in TTT_RandomPlayer and TBS_HumanPlayer
	virtual void OnTurn() override;
	virtual void OnWin() override;
	virtual void OnLose() override;
	virtual void OnPlacement() override;
};