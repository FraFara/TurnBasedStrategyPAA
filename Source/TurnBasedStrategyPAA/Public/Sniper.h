// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Sniper.generated.h"

/**
 *
 */
UCLASS()
class TURNBASEDSTRATEGYPAA_API ASniper : public AUnit
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ASniper();

    // Override the Attack method to implement CounterAttack
    virtual int32 Attack(AUnit* TargetUnit) override;
};