// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Unit.h"
#include "Grid.h"
#include "TBS_PlayerInterface.h"
#include "TBS_GameMode.generated.h"

// Define an enum for game phases
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	NONE        UMETA(DisplayName = "None"),
	SETUP		UMETA(DisplayName = "Setup"),
	GAMEPLAY	UMETA(DisplayName = "Gameplay")
};

/**
 * Game Mode for the Turn-Based Strategy game
 */
UCLASS()
class TURNBASEDSTRATEGYPAA_API ATBS_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATBS_GameMode();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Reference to the grid
	UPROPERTY(Transient)
	AGrid* GameGrid;

	// TSubclassOf is a template class that provides UClass type safety.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGrid> GridClass;

	// Grid size
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
	int32 GridSize;

	// tracks if the game is over
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Rules")
	bool bIsGameOver;

	// Array of player actors
	UPROPERTY(Transient)
	TArray<AActor*> Players;

	// Identifier for the current player
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Rules")
	int32 CurrentPlayer;

	// Number of players -> it will not really change, but it's useful for some functions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
	int32 NumberOfPlayers;

	// Units per player
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
	int32 UnitsPerPlayer;

	// Obstacles percentage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules") // implementare un limite?
	float ObstaclePercentage;

	// Types of units
	UPROPERTY(EditDefaultsOnly, Category = "Playing Units")
	TSubclassOf<AUnit> BrawlerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Playing Units")
	TSubclassOf<AUnit> SniperClass;

	// Current action
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	EGamePhase CurrentPhase;

	// Units remaining per player
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Playing Units")
	TArray<int32> UnitsRemaining;

	// Units placed so far
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Playing Units")
	int32 UnitsPlaced;

	// Simulate coin toss to determine who starts
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	int32 SimulateCoinToss();

	// Start the placement phase
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void StartPlacementPhase(int32 StartingPlayer);

	// Start gameplay phase
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void StartGameplayPhase();

	// End current player's turn and switch to next player
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void EndTurn();

	// Place a unit at the specified tile
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	bool PlaceUnit(EUnitType Type, int32 GridX, int32 GridY, int32 PlayerIndex);

	// Check if the game is over
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	bool CheckGameOver();

	// Notify a unit was destroyed
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void NotifyUnitDestroyed(int32 PlayerIndex);

	// Get the current player
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	AActor* GetCurrentPlayer();

	// Call when a player wins
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void PlayerWon(int32 PlayerIndex);

};