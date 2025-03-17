// Fill out your copyright notice in the Description page of Project Settings.


#include "TBS_HumanPlayer.h"
#include "Grid.h"
#include "Sniper.h"
#include "Brawler.h"
#include "TBS_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// Sets default values
ATBS_HumanPlayer::ATBS_HumanPlayer()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Set this pawn to be controlled by the lowest-numbered player
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// creates a camera component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	// Sets the camera as RootComponent
	SetRootComponent(Camera);
	// Camera Placement -> Top Down View
	Camera->SetRelativeLocation(FVector(0, 0, 200));
	Camera->SetRelativeRotation(FRotator(-90, 0, 0));
	// default init values
	PlayerNumber = 0;
	UnitColor = EUnitColor::BLUE;
	IsMyTurn = false;
	SelectedUnit = nullptr;
	// Initializes current action to None
	CurrentAction = EPlayerAction::NONE;

}

// Called when the game starts or when spawned
void ATBS_HumanPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATBS_HumanPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATBS_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind the OnClick function to left mouse click
	PlayerInputComponent->BindAction("LeftMouseClick", IE_Pressed, this, &ATBS_HumanPlayer::OnClick);

	// Additional bindings for movement, attack, skip
	PlayerInputComponent->BindAction("MoveAction", IE_Pressed, this, &ATBS_HumanPlayer::HighlightMovementTiles);
	PlayerInputComponent->BindAction("AttacAction", IE_Pressed, this, &ATBS_HumanPlayer::HighlightAttackTiles);
	PlayerInputComponent->BindAction("SkipUnitTurn", IE_Pressed, this, &ATBS_HumanPlayer::SkipUnitTurn);
	PlayerInputComponent->BindAction("EndTurn", IE_Pressed, this, &ATBS_HumanPlayer::EndTurn);
	//PlayerInputComponent->BindAction("ChangeCamera", IE_Pressed, this, &ATBS_HumanPlayer::ChangeCameraPosition);

}

void ATBS_HumanPlayer::OnTurn()
{
	IsMyTurn = true;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Your Turn"));
}

void ATBS_HumanPlayer::OnWin()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("You Win!"));
}

void ATBS_HumanPlayer::OnLose()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("You Lose!"));
}

void ATBS_HumanPlayer::OnPlacement()
{
	IsMyTurn = true;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Place Your Units"));
}

void ATBS_HumanPlayer::FindMyUnits()
{
	MyUnits.Empty();

	// Find all units of this player
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		AUnit* Unit = Cast<AUnit>(Actor);
		if (Unit && Unit->GetOwnerID() == PlayerNumber)
		{
			MyUnits.Add(Unit);
		}
	}
}

void ATBS_HumanPlayer::OnClick()
{
	// Get game instance
	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	//Structure containing information about one hit of a trace, such as point of impact and surface normal at that point
	FHitResult Hit = FHitResult(ForceInit);
	// GetHitResultUnderCursor function sends a ray from the mouse position
	GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(ECollisionChannel::ECC_Pawn, true, Hit);

	// If something is hit...
	if (Hit.bBlockingHit)
	{
		// Check if it's a tile
		ATile* ClickedTile = Cast<ATile>(Hit.GetActor());

		if (ClickedTile)
		{
			// If there's a unit on that tile get it
			AUnit* UnitOnTile = ClickedTile->GetOccupyingUnit();

			// Handle based on current action
			switch (CurrentAction)
			{
			case EPlayerAction::NONE:

				// If clicked a unit, show info and possibly select it
				if (UnitOnTile)
				{
					// Update turn message with unit health info
					GameInstance->SetTurnMessage(UnitOnTile->GetLiveHealth());

					// If it's our unit, select it
					if (UnitOnTile->GetOwnerID() == PlayerNumber)
					{
						// If already selected, deselect it
						if (SelectedUnit == UnitOnTile)
						{
							SelectedUnit = nullptr;
							GameInstance->SetTurnMessage(TEXT("Unit deselected"));
						}
						else
						{
							SelectedUnit = UnitOnTile;
							
							FString SelectMessage = FString::Printf(TEXT("Selected %s"), *UnitOnTile->GetUnitName());
							GameInstance->SetTurnMessage(SelectMessage);

							// Print avaiable actions
							FString ActionMsg;
							if (!UnitOnTile->HasMoved())
							{
								ActionMsg += TEXT("Movement available. ");
							}
							if (!UnitOnTile->HasAttacked())
							{
								ActionMsg += TEXT("Attack available. ");
							}
							if (!UnitOnTile->HasAttacked() || !UnitOnTile->HasMoved())
							{
								ActionMsg += TEXT("Unit can Skip Turn.");
							}

							if (!ActionMsg.IsEmpty())
							{
								GameInstance->SetTurnMessage(ActionMsg);
							}
							
						}
					}
				}
				break;

			case EPlayerAction::MOVEMENT:
				// If in movement mode, try to move the selected unit
				if (SelectedUnit && !SelectedUnit->HasMoved())
				{
					if (HighlightedMovementTiles.Contains(ClickedTile))
					{
						// Record initial position for move history
						FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
						FVector2D ToPos = ClickedTile->GetGridPosition();

						// Move the unit
						SelectedUnit->MoveToTile(ClickedTile);

						if (GameInstance)
						{
							// Log the movement in history
							GameInstance->AddMoveToHistory(PlayerNumber, SelectedUnit->GetUnitName(), TEXT("Move"), FromPos, ToPos,0);

							GameInstance->SetTurnMessage(TEXT("Unit moved"));
						}

						// Get game mode to record the move
						ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
						
						GameMode->RecordMove(PlayerNumber, SelectedUnit->GetUnitName(), TEXT("Move"), FromPos, ToPos, 0);
						

						// Clear highlighted tiles
						ClearHighlightedTiles();

						// If the unit can still attack, show message
						if (!SelectedUnit->HasAttacked())
						{
							
							GameInstance->SetTurnMessage(TEXT("Attack available"));
							
						}
						else
						{
							// Unit's action is over, deselect
							SelectedUnit = nullptr;
						}
					}
					// Invalid move target
					else
					{
						GameInstance->SetTurnMessage(TEXT("Cannot move there"));

					}
				}
				break;

			case EPlayerAction::ATTACK:
				// If in attack mode, try to attack
				if (SelectedUnit && !SelectedUnit->HasAttacked())
				{
					if (HighlightedAttackTiles.Contains(ClickedTile) && UnitOnTile)
					{
						// Get positions for move history
						FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
						FVector2D TargetPos = ClickedTile->GetGridPosition();

						// Attack the unit
						int32 Damage = SelectedUnit->Attack(UnitOnTile);

						// Show attack result
						if (Damage > 0)
						{
							if (GameInstance)
							{
								// Log the attack in history
								GameInstance->AddMoveToHistory(PlayerNumber, SelectedUnit->GetUnitName(), TEXT("Attack"), FromPos, TargetPos, Damage);

								FString AttackMessage = FString::Printf(TEXT("%s dealt %d damage to %s"),
									*SelectedUnit->GetUnitName(), Damage, *UnitOnTile->GetUnitName());
								GameInstance->SetTurnMessage(AttackMessage);
							}

							// Get game mode to record the move
							ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
							if (GameMode)
							{
								GameMode->RecordMove(PlayerNumber, SelectedUnit->GetUnitName(), TEXT("Attack"), FromPos, TargetPos, Damage);
							}
						}

						// Clear highlighted tiles
						ClearHighlightedTiles();

						// Unit's action is over, deselect
						SelectedUnit = nullptr;
					}
					else
					{
						// Invalid attack target
						if (GameInstance)
						{
							GameInstance->SetTurnMessage(TEXT("Cannot attack there"));
						}
					}
				}
				break;

			case EPlayerAction::PLACEMENT:
				// Placement logic using GameMode
				if (ClickedTile->GetTileStatus() == ETileStatus::EMPTY)
				{
					FVector2D GridPos = ClickedTile->GetGridPosition();
					int32 GridX = GridPos.X;
					int32 GridY = GridPos.Y;

					// Get the game mode to place unit
					ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
					if (GameMode && UnitToPlace != EUnitType::NONE)
					{
						bool Success = GameMode->PlaceUnit(UnitToPlace, GridX, GridY, PlayerNumber);

						if (Success)
						{
							// Record the placement in history
							FString UnitTypeString = (UnitToPlace == EUnitType::BRAWLER) ? TEXT("Brawler") : TEXT("Sniper");
							GameInstance->AddMoveToHistory(PlayerNumber, UnitTypeString, TEXT("Place"), FVector2D::ZeroVector, GridPos, 0);

							FString PlacementMsg = FString::Printf(TEXT("Placed %s at (%d, %d)"), *UnitTypeString, GridX, GridY);
							GameInstance->SetTurnMessage(PlacementMsg);

							// Reset unit selection
							UnitToPlace = EUnitType::NONE;
						}
					}
					else
					{
						
						GameInstance->SetTurnMessage(TEXT("Select unit type to place"));
						
					}
				}
				else
				{
					
					GameInstance->SetTurnMessage(TEXT("Cannot place unit here"));
					
				}
				break;
			}
		}
	}
}

void ATBS_HumanPlayer::OnRightClick()
{
	// Cancel the current action
	if (CurrentAction != EPlayerAction::NONE)
	{
		// Clear highlighted tiles
		ClearHighlightedTiles();

		// Reset action
		CurrentAction = EPlayerAction::NONE;

		// Show message to player
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Action canceled"));
	}
	// If no action is active but a unit is selected, deselect it
	else if (SelectedUnit)
	{
		SelectedUnit = nullptr;

		ClearHighlightedTiles();

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Unit deselected"));
	}
}


void ATBS_HumanPlayer::SkipUnitTurn()
{
	if (SelectedUnit && IsMyTurn)
	{
		// Mark the unit as having moved and attacked
		SelectedUnit->bHasMoved = true;
		SelectedUnit->bHasAttacked = true;

		// Display a message
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
			FString::Printf(TEXT("%s turn skipped"), *SelectedUnit->GetUnitName()));

		// Clears the selection
		SelectedUnit = nullptr;
	}
}

	void ATBS_HumanPlayer::EndTurn()
	{
	}

	void ATBS_HumanPlayer::HighlightMovementTiles()
	{
		if (!IsMyTurn || !SelectedUnit || SelectedUnit->HasMoved())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot move this unit"));
			return;
		}

		// Clear any existing highlights
		ClearHighlightedTiles();

		// Get movement tiles
		HighlightedMovementTiles = SelectedUnit->GetMovementTiles();

		// Apply visual highlight to each tile (this is where you'd add material changes)
		for (ATile* Tile : HighlightedMovementTiles)
		{
			// Add visual highlight here - for now just debug msg
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green,
				FString::Printf(TEXT("Movement tile: (%d, %d)"),
					(int32)Tile->GetGridPosition().X, (int32)Tile->GetGridPosition().Y));
		}

		// Set action
		CurrentAction = EPlayerAction::MOVEMENT;

		// Highlight Tiles IMPLEMENTA https://www.youtube.com/watch?v=R7oLZL97XYo&ab_channel=BuildGamesWithJon
	}

	void ATBS_HumanPlayer::HighlightAttackTiles()
	{
		if (!IsMyTurn || !SelectedUnit || SelectedUnit->HasAttacked())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot attack with this unit"));
			return;
		}

		// Clear any existing highlights
		ClearHighlightedTiles();

		// Get attack tiles
		HighlightedAttackTiles = SelectedUnit->GetAttackTiles();

		// Apply visual highlight to each tile (this is where you'd add material changes)
		for (ATile* Tile : HighlightedAttackTiles)
		{
			// Add visual highlight here - for now just debug msg
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red,
				FString::Printf(TEXT("Attack tile: (%d, %d)"),
					(int32)Tile->GetGridPosition().X, (int32)Tile->GetGridPosition().Y));
		}

		// Set mode
		CurrentAction = EPlayerAction::ATTACK;

		// Highlight Tiles IMPLEMENTA https://www.youtube.com/watch?v=R7oLZL97XYo&ab_channel=BuildGamesWithJon
	}

	void ATBS_HumanPlayer::ClearHighlightedTiles()
	{
		// Clear visual highlights (this is where you'd remove material changes)
		for (ATile* Tile : HighlightedMovementTiles)
		{
			// Remove visual highlight here
		}

		for (ATile* Tile : HighlightedAttackTiles)
		{
			// Remove visual highlight here
		}

		// Clear arrays
		HighlightedMovementTiles.Empty();
		HighlightedAttackTiles.Empty();

		// Reset mode
		CurrentAction = EPlayerAction::NONE;
	}

	void ATBS_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, EUnitType Type)
	{
		// Implementation would connect to GameMode's PlaceUnit function
		// Get the game mode
		AGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode();
		if (GameModeBase)
		{
			// Cast to game mode
			ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GameModeBase);
			//Place the unit through the game mode
			GameMode->PlaceUnit(Type, GridX, GridY, PlayerNumber);
		}
	}



	//void ATBS_HumanPlayer::ChangeCameraPosition()
	//{

	// Default top-down view
	//Camera->SetRelativeLocation(FVector(0, 0, 200));
	//Camera->SetRelativeRotation(FRotator(-90, 0, 0));

	// Isometric view from one corner
	//		Camera->SetRelativeLocation(FVector(-200, -200, 150));
	//		Camera->SetRelativeRotation(FRotator(-45, 45, 0));

	// Isometric view from opposite corner
	//		Camera->SetRelativeLocation(FVector(200, 200, 150));
	//		Camera->SetRelativeRotation(FRotator(-45, 225, 0));

	//}


