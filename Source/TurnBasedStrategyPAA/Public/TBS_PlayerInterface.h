// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Unit.h"
#include "TBS_PlayerInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTBS_PlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TURNBASEDSTRATEGYPAA_API ITBS_PlayerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	EUnitColor UnitColor;
	bool IsHumanTurn = false;
	int32 PlayerNumber;

	// Virtual function to start the turn and show a custom message
	virtual void OnTurn() {};
	// Virtual function to show a message when the player win
	virtual void OnWin() {};
	// Virtual function to show a message when the player lose "Attackable tiles" in the player interfae
	virtual void OnLose() {};
	// Virtual function for placement phase
	virtual void OnPlacement() {};

};
