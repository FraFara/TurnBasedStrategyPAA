// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TBS_HumanPlayer.h"
#include "TBS_GameInstance.h"

ATBS_PlayerController::ATBS_PlayerController()
{
    // Setup controller defaults
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ATBS_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Set up enhanced input
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(TBSContext, 0);
    }

    //// Create and show the game HUD
    //if (GameHUDClass)
    //{
    //    GameHUD = CreateWidget<UUserWidget>(this, GameHUDClass);
    //    ShowHUD();
    //}
}

void ATBS_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ATBS_PlayerController::ClickOnGrid);
        EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Triggered, this, &ATBS_PlayerController::CancelAction);
    }
}

void ATBS_PlayerController::ClickOnGrid()
{
    const auto HumanPlayer = Cast<ATBS_HumanPlayer>(GetPawn());
    if (IsValid(HumanPlayer))
    {
        HumanPlayer->OnClick();
    }
}

void ATBS_PlayerController::CancelAction()
{
    const auto HumanPlayer = Cast<ATBS_HumanPlayer>(GetPawn());
    if (IsValid(HumanPlayer))
    {
        HumanPlayer->OnRightClick();
    }
}

void ATBS_PlayerController::ShowHUD()
{
    //if (GameHUD && GameHUD->IsValidLowLevel())
    //{
    //    GameHUD->AddToViewport();

    //    // Set input mode to game and UI
    //    FInputModeGameAndUI InputMode;
    //    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    //    InputMode.SetHideCursorDuringCapture(false);
    //    SetInputMode(InputMode);
    //}
}

void ATBS_PlayerController::OnGameOver(bool bPlayerWon)
{
    // Handle game over state - typically show end game UI elements

    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->RecordGameResult(bPlayerWon);

        // Example of showing a message - in a full implementation,
        // you would likely use a dedicated end game widget
        FString ResultMessage = bPlayerWon ? "You Won!" : "You Lost!";
        GEngine->AddOnScreenDebugMessage(-1, 10.f, bPlayerWon ? FColor::Green : FColor::Red, ResultMessage);
    }
}