// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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

    // Set the input mode to game only (no UI)
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}

void ATBS_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Ensure enhanced input is used
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        // Bind actions more explicitly
        EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ATBS_PlayerController::ClickOnGrid);
        EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Triggered, this, &ATBS_PlayerController::CancelAction);
    }
}

void ATBS_PlayerController::SetGameInputMode()
{
    // Ensure game input mode is set
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    bShowMouseCursor = true;

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        TEXT("Input mode set to Game"));
}

void ATBS_PlayerController::ClickOnGrid()
{
    // Robust error checking
    APawn* ControlledPawn = GetPawn();
    ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(ControlledPawn);

    if (HumanPlayer)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Clicking on Grid"));
        HumanPlayer->OnClick();
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to cast to HumanPlayer"));
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

void ATBS_PlayerController::OnGameOver(bool bPlayerWon)
{
    // Handle game over state
    UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
  
    GameInstance->RecordGameResult(bPlayerWon);

    // Show a message using screen debug messages
    FString ResultMessage = bPlayerWon ? "You Won!" : "You Lost!";
    GEngine->AddOnScreenDebugMessage(-1, 10.f, bPlayerWon ? FColor::Green : FColor::Red, ResultMessage);
    
}