// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "TBS_HumanPlayer.h"
#include "TBS_PlayerController.generated.h"

/**
 * Player controller for the turn-based strategy game
 */
UCLASS()
class ATBS_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATBS_PlayerController();

    // Enhanced Input context and actions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputMappingContext* TBSContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction* ClickAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction* RightClickAction;

    // Called when clicking on the grid
    void ClickOnGrid();

    // Called when right-clicking (to cancel actions)
    void CancelAction();

    // Notification methods for game events
    UFUNCTION(BlueprintCallable, Category = "Game")
    void OnGameOver(bool bPlayerWon);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
};