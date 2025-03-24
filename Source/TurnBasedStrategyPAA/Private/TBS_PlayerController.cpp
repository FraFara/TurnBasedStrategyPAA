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
}

void ATBS_PlayerController::ClickOnGrid()
{
    // Get hit results for all objects
    FHitResult HitResult;
    bool bHitSuccessful = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    if (bHitSuccessful)
    {
        // Try to cast to tile first
        ATile* ClickedTile = Cast<ATile>(HitResult.GetActor());

        // If no tile found, try to find the closest tile
        if (!ClickedTile)
        {
            TArray<AActor*> FoundTiles;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATile::StaticClass(), FoundTiles);

            float ClosestDistance = FLT_MAX;
            for (AActor* TileActor : FoundTiles)
            {
                ATile* Tile = Cast<ATile>(TileActor);
                if (Tile)
                {
                    float Distance = FVector::Distance(HitResult.Location, Tile->GetActorLocation());
                    if (Distance < ClosestDistance)
                    {
                        ClosestDistance = Distance;
                        ClickedTile = Tile;
                    }
                }
            }
        }

        // If we found a tile, pass it to the human player
        if (ClickedTile)
        {
            APawn* ControlledPawn = GetPawn();
            ATBS_HumanPlayer* HumanPlayer = Cast<ATBS_HumanPlayer>(ControlledPawn);

            if (HumanPlayer)
            {
                // Use the tile we found
                HumanPlayer->OnClick();
            }
        }
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