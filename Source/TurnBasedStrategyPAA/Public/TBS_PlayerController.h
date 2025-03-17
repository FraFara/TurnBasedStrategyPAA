// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "TBS_HumanPlayer.h"
#include "Blueprint/UserWidget.h"
#include "TBS_PlayerController.generated.h"

/**
 *
 */
UCLASS()
class ATBS_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATBS_PlayerController();

    // Enhanced Input context and actions
    UPROPERTY(EditAnywhere, Category = Input)
    UInputMappingContext* TBSContext;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* ClickAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* RightClickAction;

    // Called when clicking on the grid
    void ClickOnGrid();

    // Called when right-clicking (to cancel actions)
    void CancelAction();

    //// Game HUD reference
    //UPROPERTY(EditDefaultsOnly, Category = "UI")
    //TSubclassOf<UUserWidget> GameHUDClass;

    //UPROPERTY()
    //UUserWidget* GameHUD;

    // Function to show the HUD
    UFUNCTION(BlueprintCallable)
    void ShowHUD();

    // Notification methods for game events
    void OnGameOver(bool bPlayerWon);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
};