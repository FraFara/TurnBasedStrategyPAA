// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TBS_PlayerInterface.generated.h"


UINTERFACE(MinimalAPI)
class UTBS_PlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Player interface used by both human and AI players
 */
class TURNBASEDSTRATEGYPAA_API ITBS_PlayerInterface
{
	GENERATED_BODY()

public:
	// Called when the player can place units
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Game")
	void OnPlacement();

	// Called when the player can take their turn
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Game")
	void OnTurn();

	// Called when the player wins
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Game")
	void OnWin();

	// Called when the player loses
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Game")
	void OnLose();

	// Explicitly set turn state
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Turn Management")
	void SetTurnState(bool bIsMyTurn);

	// Method to update UI with current state
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UI")
	void UpdateUI();
};