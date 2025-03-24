// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AISelectionWidget.generated.h"

/**
 * Widget for selecting AI difficulty level
 */
UCLASS()
class TURNBASEDSTRATEGYPAA_API UAISelectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Event dispatcher for AI selection
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIDifficultySelected, bool, bIsHardAI);

    UPROPERTY(BlueprintAssignable, Category = "AI Selection")
    FOnAIDifficultySelected OnAIDifficultySelected;

    // Called when the Easy AI button is clicked
    UFUNCTION(BlueprintCallable, Category = "AI Selection")
    void OnEasyAISelected();

    // Called when the Hard AI button is clicked
    UFUNCTION(BlueprintCallable, Category = "AI Selection")
    void OnHardAISelected();
};