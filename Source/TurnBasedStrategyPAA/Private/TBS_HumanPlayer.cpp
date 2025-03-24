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

	// Get the game instance
	GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

	// Initialize UnitToPlace to NONE
	UnitToPlace = EUnitType::NONE;

	// Log that the human player is ready
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Human Player Initialized"));
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

}

void ATBS_HumanPlayer::OnTurn_Implementation()
{
	IsMyTurn = true;

	// Find all your units at the start of your turn
	FindMyUnits();

	// Ensure game input mode
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	// Update UI
	if (GameInstance)
	{
		GameInstance->SetTurnMessage(TEXT("Your Turn - Select a unit"));
	}

	// Debug each unit found
	for (AUnit* Unit : MyUnits)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
			FString::Printf(TEXT("Your Unit: %s at (%f,%f)"),
				*Unit->GetUnitName(),
				Unit->GetCurrentTile()->GetGridPosition().X,
				Unit->GetCurrentTile()->GetGridPosition().Y));
	}
}

void ATBS_HumanPlayer::OnWin_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("You Win!"));

	// Ensure the game instance is set
	if (!GameInstance)
	{
		GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	}

	if (GameInstance)
	{
		// Set the winner to the human player (index 0)
		GameInstance->SetWinner(0);
	}
}

void ATBS_HumanPlayer::OnLose_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("You Lose!"));
}

void ATBS_HumanPlayer::OnPlacement_Implementation()
{
	// Check if this is really our turn (match with game mode's current player)
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode && GameMode->CurrentPlayer != PlayerNumber)
	{
		// It's not actually our turn - don't do anything
		IsMyTurn = false;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not human player's turn for placement"));
		return;
	}

	// If we get here, it's definitely our turn
	IsMyTurn = true;
	CurrentAction = EPlayerAction::PLACEMENT;

	// Set message
	// Check if GameInstance is valid before using it
	if (!GameInstance)
	{
		GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	}

	// Set message only if GameInstance is valid
	if (GameInstance)
	{
		GameInstance->SetTurnMessage(TEXT("Place Your Units"));
	}

	if (GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Showing unit selection UI for human player"));
		GameMode->ShowUnitSelectionUI(true);
	}
}

void ATBS_HumanPlayer::FindMyUnits()
{
	MyUnits.Empty();

	// Find all units of this player
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Total Units Found: %d"), AllUnits.Num()));


	for (AActor* Actor : AllUnits)
	{
		AUnit* Unit = Cast<AUnit>(Actor);
		if (Unit && Unit->GetOwnerID() == PlayerNumber)
		{
			MyUnits.Add(Unit);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
				FString::Printf(TEXT("My Unit: %s"), *Unit->GetUnitName()));
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
		FString::Printf(TEXT("My Units Count: %d"), MyUnits.Num()));
}

void ATBS_HumanPlayer::OnClick()
{
	// Get the game mode for reference
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !GameMode->GameGrid)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GameMode or Grid is null in OnClick"));
		return;
	}

	// Get hit result under cursor
	FHitResult Hit;
	bool bHitSuccessful = GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(
		ECollisionChannel::ECC_Visibility, false, Hit);

	if (!bHitSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No hit under cursor"));
		return;
	}

	// Prioritize direct hits on Tile or Unit
	ATile* ClickedTile = Cast<ATile>(Hit.GetActor());
	AUnit* ClickedUnit = Cast<AUnit>(Hit.GetActor());

	// If a unit was clicked, get its tile
	if (ClickedUnit)
	{
		ClickedTile = ClickedUnit->GetCurrentTile();
	}

	// If no tile found through direct hit, try grid conversion
	if (!ClickedTile)
	{
		AGrid* Grid = GameMode->GameGrid;
		FVector2D GridPos = Grid->GetXYPositionByRelativeLocation(Hit.Location);
		int32 GridX = FMath::FloorToInt(GridPos.X);
		int32 GridY = FMath::FloorToInt(GridPos.Y);

		// Check if grid position is valid
		if (GridX >= 0 && GridX < Grid->Size && GridY >= 0 && GridY < Grid->Size)
		{
			FVector2D TilePos(GridX, GridY);
			if (Grid->TileMap.Contains(TilePos))
			{
				ClickedTile = Grid->TileMap[TilePos];
			}
		}
	}

	// Validate the clicked tile
	if (!ClickedTile)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to find a valid tile"));
		return;
	}

	// EXPLICIT CHECK: If in GAMEPLAY but CurrentAction is PLACEMENT, reset it
	if (GameMode->CurrentPhase == EGamePhase::GAMEPLAY && CurrentAction == EPlayerAction::PLACEMENT)
	{
		CurrentAction = EPlayerAction::NONE;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Fixing action state: Was in PLACEMENT during GAMEPLAY"));
	}

	// Handle Setup/Placement Phase
	if (GameMode->CurrentPhase == EGamePhase::SETUP)
	{
		// Validate turn readiness
		if (!IsMyTurn)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("Cannot place unit. Not your turn."));
			return;
		}

		// Check tile status
		if (ClickedTile->GetTileStatus() != ETileStatus::EMPTY)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("Selected tile is not empty"));
			return;
		}

		// Set the current placement tile
		CurrentPlacementTile = ClickedTile;

		// Detailed debug logging
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("Tile selected for placement at (%f, %f)"),
				ClickedTile->GetGridPosition().X,
				ClickedTile->GetGridPosition().Y));

		// Show unit selection UI
		GameMode->ShowUnitSelectionUI(true);

		return;
	}

	// Handle Gameplay Phase
	else if (GameMode->CurrentPhase == EGamePhase::GAMEPLAY)
	{
		// Validate turn readiness
		if (!IsMyTurn)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("Cannot select unit. Not your turn. IsMyTurn=%d, CurrentPlayer=%d"),
					IsMyTurn ? 1 : 0, GameMode->CurrentPlayer));
			return;
		}

		// Get the unit on the tile
		AUnit* UnitOnTile = ClickedTile->GetOccupyingUnit();

		// Enhanced debug
		if (UnitOnTile)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
				FString::Printf(TEXT("Tile contains unit: %s, Owner: %d, Health: %d/%d"),
					*UnitOnTile->GetUnitName(), UnitOnTile->GetOwnerID(),
					UnitOnTile->GetUnitHealth(), UnitOnTile->GetMaxHealth()));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
				TEXT("Clicked on empty tile"));
		}

		// Gameplay action logic
		switch (CurrentAction)
		{
		case EPlayerAction::NONE:
			// Unit selection logic
			if (UnitOnTile)
			{
				// Always update LastClickedUnit for HUD display
				LastClickedUnit = UnitOnTile;

				// Log player comparing to ensure ownership check is correct
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
					FString::Printf(TEXT("Unit owner: %d, Your player number: %d"),
						UnitOnTile->GetOwnerID(), PlayerNumber));

				// Unit selection logic - ensure ownership check is correct
				if (UnitOnTile->GetOwnerID() == PlayerNumber)
				{
					// Update selected unit
					if (SelectedUnit == UnitOnTile)
					{
						// Deselect if already selected
						SelectedUnit = nullptr;
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Unit deselected"));
					}
					else
					{
						SelectedUnit = UnitOnTile;
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
							FString::Printf(TEXT("Unit selected: %s"), *UnitOnTile->GetUnitName()));
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
						TEXT("Cannot select enemy unit!"));
				}
			}
			break;

		case EPlayerAction::MOVEMENT:
			if (SelectedUnit && !SelectedUnit->HasMoved())
			{
				TArray<ATile*> MovementTiles = SelectedUnit->GetMovementTiles();

				// More robust tile matching
				ATile* MatchedTile = nullptr;
				for (ATile* MovementTile : MovementTiles)
				{
					// Compare grid positions precisely
					if (MovementTile->GetGridPosition().X == ClickedTile->GetGridPosition().X &&
						MovementTile->GetGridPosition().Y == ClickedTile->GetGridPosition().Y)
					{
						MatchedTile = MovementTile;
						break;
					}
				}

				if (MatchedTile)
				{
					FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
					FVector2D ToPos = MatchedTile->GetGridPosition();

					if (SelectedUnit->MoveToTile(MatchedTile))
					{
						// Existing move history and UI update logic
						if (GameInstance)
						{
							GameInstance->AddMoveToHistory(PlayerNumber,
								SelectedUnit->GetUnitName(),
								TEXT("Move"), FromPos, ToPos, 0);
						}

						ClearHighlightedTiles();

						// Check if unit can still attack
						if (!SelectedUnit->HasAttacked() && GameInstance)
						{
							GameInstance->SetTurnMessage(TEXT("Attack available"));
						}
						else
						{
							SelectedUnit = nullptr;
							CheckAllUnitsFinished();
						}
					}
				}
				else
				{
					// More detailed error logging
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
						FString::Printf(TEXT("Cannot move to tile at (%f, %f)"),
							ClickedTile->GetGridPosition().X,
							ClickedTile->GetGridPosition().Y));

					// Print out all valid movement tiles for comparison
					for (ATile* MovementTile : MovementTiles)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
							FString::Printf(TEXT("Valid Movement Tile: (%f, %f)"),
								MovementTile->GetGridPosition().X,
								MovementTile->GetGridPosition().Y));
					}
				}
			}
			break;

		case EPlayerAction::ATTACK:
			// Attack logic
			if (SelectedUnit && !SelectedUnit->HasAttacked() &&
				HighlightedAttackTiles.Contains(ClickedTile) && UnitOnTile)
			{
				FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
				FVector2D TargetPos = ClickedTile->GetGridPosition();

				int32 Damage = SelectedUnit->Attack(UnitOnTile);

				if (Damage > 0 && GameInstance)
				{
					// Record attack in history
					GameInstance->AddMoveToHistory(PlayerNumber,
						SelectedUnit->GetUnitName(),
						TEXT("Attack"), FromPos, TargetPos, Damage);

					if (GameMode)
					{
						GameMode->RecordMove(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Attack"), FromPos, TargetPos, Damage);
					}

					GameInstance->SetTurnMessage(
						FString::Printf(TEXT("%s dealt %d damage to %s"),
							*SelectedUnit->GetUnitName(), Damage, *UnitOnTile->GetUnitName()));
				}

				ClearHighlightedTiles();
				SelectedUnit = nullptr;

				// Check if all units have completed their actions
				CheckAllUnitsFinished();
			}
			break;
		}
	}
}

//void ATBS_HumanPlayer::OnClick()
//{
//	// Get the game mode for reference
//	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
//	if (!GameMode || !GameMode->GameGrid)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GameMode or Grid is null in OnClick"));
//		return;
//	}
//
//	// Get hit result under cursor
//	FHitResult Hit;
//	bool bHitSuccessful = GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(
//		ECollisionChannel::ECC_Visibility, false, Hit);
//
//	if (!bHitSuccessful)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No hit under cursor"));
//		return;
//	}
//
//	// Try to cast to a tile
//	ATile* ClickedTile = Cast<ATile>(Hit.GetActor());
//
//	if (!ClickedTile)
//	{
//		// Fallback to converting the hit location to grid coordinates
//		AGrid* Grid = GameMode->GameGrid;
//		FVector2D GridPos = Grid->GetXYPositionByRelativeLocation(Hit.Location);
//		int32 GridX = FMath::FloorToInt(GridPos.X);
//		int32 GridY = FMath::FloorToInt(GridPos.Y);
//
//		// Check if grid position is valid
//		if (GridX < 0 || GridX >= Grid->Size || GridY < 0 || GridY >= Grid->Size)
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Grid position out of bounds"));
//			return;
//		}
//
//		// Get the tile at this grid position
//		FVector2D TilePos(GridX, GridY);
//		if (!Grid->TileMap.Contains(TilePos))
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No tile at this grid position"));
//			return;
//		}
//
//		ClickedTile = Grid->TileMap[TilePos];
//	}
//
//	if (!ClickedTile)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to find a valid tile"));
//		return;
//	}
//
//	// Debug the clicked tile
//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
//		FString::Printf(TEXT("Clicked on tile at (%f,%f)"),
//			ClickedTile->GetGridPosition().X, ClickedTile->GetGridPosition().Y));
//
//	// Now handle different cases based on current action
//	switch (CurrentAction)
//	{
//	case EPlayerAction::MOVEMENT:
//		// If we're in movement mode and have a selected unit
//		if (SelectedUnit && !SelectedUnit->HasMoved())
//		{
//			// First check if this tile is in our highlighted movement tiles
//			if (HighlightedMovementTiles.Contains(ClickedTile))
//			{
//				FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
//				FVector2D ToPos = ClickedTile->GetGridPosition();
//
//				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
//					FString::Printf(TEXT("Moving unit from (%f,%f) to (%f,%f)"),
//						FromPos.X, FromPos.Y, ToPos.X, ToPos.Y));
//
//				if (SelectedUnit->MoveToTile(ClickedTile))
//				{
//					// Record move in history
//					if (GameInstance)
//					{
//						GameInstance->AddMoveToHistory(PlayerNumber,
//							SelectedUnit->GetUnitName(),
//							TEXT("Move"), FromPos, ToPos, 0);
//					}
//
//					ClearHighlightedTiles();
//
//					// Check if unit can still attack
//					if (!SelectedUnit->HasAttacked() && GameInstance)
//					{
//						GameInstance->SetTurnMessage(TEXT("Attack available"));
//					}
//					else
//					{
//						SelectedUnit = nullptr;
//						// Check if all units have completed their actions
//						CheckAllUnitsFinished();
//					}
//				}
//				else
//				{
//					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Failed to move unit to tile"));
//				}
//			}
//			else
//			{
//				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Selected tile is not in movement range"));
//			}
//		}
//		else
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
//				FString::Printf(TEXT("Cannot move: SelectedUnit=%s, HasMoved=%s"),
//					SelectedUnit ? TEXT("valid") : TEXT("null"),
//					SelectedUnit && SelectedUnit->HasMoved() ? TEXT("true") : TEXT("false")));
//		}
//		break;
//
//	case EPlayerAction::ATTACK:
//		// If we're in attack mode and have a selected unit
//		if (SelectedUnit && !SelectedUnit->HasAttacked())
//		{
//			// Check if target tile is in highlighted attack tiles
//			if (HighlightedAttackTiles.Contains(ClickedTile))
//			{
//				// Get the unit on the target tile
//				AUnit* TargetUnit = ClickedTile->GetOccupyingUnit();
//
//				if (TargetUnit)
//				{
//					FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
//					FVector2D TargetPos = ClickedTile->GetGridPosition();
//
//					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
//						FString::Printf(TEXT("Attacking unit at (%f,%f)"),
//							TargetPos.X, TargetPos.Y));
//
//					int32 Damage = SelectedUnit->Attack(TargetUnit);
//
//					if (Damage > 0 && GameInstance)
//					{
//						// Record attack in history
//						GameInstance->AddMoveToHistory(PlayerNumber,
//							SelectedUnit->GetUnitName(),
//							TEXT("Attack"), FromPos, TargetPos, Damage);
//
//						GameInstance->SetTurnMessage(
//							FString::Printf(TEXT("%s dealt %d damage to %s"),
//								*SelectedUnit->GetUnitName(), Damage, *TargetUnit->GetUnitName()));
//					}
//
//					ClearHighlightedTiles();
//					SelectedUnit = nullptr;
//
//					// Check if all units have completed their actions
//					CheckAllUnitsFinished();
//				}
//				else
//				{
//					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No unit found on target tile"));
//				}
//			}
//			else
//			{
//				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Selected tile is not in attack range"));
//			}
//		}
//		else
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
//				FString::Printf(TEXT("Cannot attack: SelectedUnit=%s, HasAttacked=%s"),
//					SelectedUnit ? TEXT("valid") : TEXT("null"),
//					SelectedUnit && SelectedUnit->HasAttacked() ? TEXT("true") : TEXT("false")));
//		}
//		break;
//
//	case EPlayerAction::NONE:
//	default:
//		// Handle unit selection
//		AUnit* UnitOnTile = ClickedTile->GetOccupyingUnit();
//
//		if (UnitOnTile)
//		{
//			// Log player comparing to ensure ownership check is correct
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
//				FString::Printf(TEXT("Unit owner: %d, Your player number: %d"),
//					UnitOnTile->GetOwnerID(), PlayerNumber));
//
//			// Unit selection logic - ensure ownership check is correct
//			if (UnitOnTile->GetOwnerID() == PlayerNumber)
//			{
//				// Update selected unit
//				if (SelectedUnit == UnitOnTile)
//				{
//					// Deselect if already selected
//					SelectedUnit = nullptr;
//					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Unit deselected"));
//				}
//				else
//				{
//					SelectedUnit = UnitOnTile;
//					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
//						FString::Printf(TEXT("Unit selected: %s"), *UnitOnTile->GetUnitName()));
//				}
//			}
//			else
//			{
//				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
//					TEXT("Cannot select enemy unit!"));
//			}
//		}
//		else
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("No unit on selected tile"));
//		}
//		break;
//	}
//}

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

void ATBS_HumanPlayer::SelectBrawlerForPlacement()
{
	UnitToPlace = EUnitType::BRAWLER;
	CurrentAction = EPlayerAction::PLACEMENT;
	IsMyTurn = true;

	GameInstance->SetTurnMessage(TEXT("Place Brawler - Click on an empty tile"));
}

void ATBS_HumanPlayer::SelectSniperForPlacement()
{
	UnitToPlace = EUnitType::SNIPER;
	CurrentAction = EPlayerAction::PLACEMENT;
	IsMyTurn = true;

	GameInstance->SetTurnMessage(TEXT("Place Sniper - Click on an empty tile"));
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

		// Check if all units have completed their actions
		CheckAllUnitsFinished();
	}
}

void ATBS_HumanPlayer::ResetActionState()
{
	CurrentAction = EPlayerAction::NONE;
	ClearCurrentPlacementTile();
}

void ATBS_HumanPlayer::CheckAllUnitsFinished()
{
	// Make sure we have the latest list of units
	FindMyUnits();

	bool allFinished = true;
	for (AUnit* Unit : MyUnits)
	{
		if (!Unit->HasMoved() || !Unit->HasAttacked())
		{
			allFinished = false;
			break;
		}
	}

	if (allFinished && MyUnits.Num() > 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("All units have finished their actions - ending turn"));

		// Get the game mode and end turn
		ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			// End turn with a slight delay to allow messages to be read
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, [this, GameMode]()
				{
					GameMode->EndTurn();
				}, 1.0f, false);
		}
	}
}

void ATBS_HumanPlayer::SetTurnState_Implementation(bool bNewTurnState)
{
	// Safely set the turn state
	IsMyTurn = bNewTurnState;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
		FString::Printf(TEXT("Human Player turn state changed to: %d"), IsMyTurn ? 1 : 0));

	// Get the game mode to check if the current player matches
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		if (IsMyTurn && GameMode->CurrentPlayer != PlayerNumber)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("WARNING: Turn state mismatch! IsMyTurn=%d but CurrentPlayer=%d"),
					IsMyTurn ? 1 : 0, GameMode->CurrentPlayer));
		}
	}

	// Use Execute_ method to call the UI update
	if (bNewTurnState)
	{
		ITBS_PlayerInterface::Execute_UpdateUI(this);

		// Make sure to find units when turn starts
		FindMyUnits();
	}
}

void ATBS_HumanPlayer::UpdateUI_Implementation()
{
	// Update "Player's Turn" text in the UI
	if (GameInstance)
	{
		FString WinnerMessage;
		if (GameInstance->HasWinner())
		{
			WinnerMessage = GameInstance->GetWinnerMessage();
		}

		if (IsMyTurn)
		{
			FString Message = TEXT("Your Turn - Select a unit");

			// If there's a winner, append it to the message
			if (!WinnerMessage.IsEmpty())
			{
				Message = Message + TEXT(" | ") + WinnerMessage;
			}

			GameInstance->SetTurnMessage(Message);
		}
		else
		{
			FString Message = TEXT("AI's Turn");

			// If there's a winner, append it to the message
			if (!WinnerMessage.IsEmpty())
			{
				Message = Message + TEXT(" | ") + WinnerMessage;
			}

			GameInstance->SetTurnMessage(Message);
		}
	}

	// Additional UI updates can be added here
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("UpdateUI called. IsMyTurn: %d"), IsMyTurn ? 1 : 0));
}


void ATBS_HumanPlayer::EndTurn()
{
}

void ATBS_HumanPlayer::HighlightMovementTiles()
{
	// Clear any previous highlights
	ClearHighlightedTiles();

	// Enhanced debug for selected unit
	if (!SelectedUnit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Cannot highlight movement tiles: No unit selected!"));
		return;
	}

	if (SelectedUnit->HasMoved())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("This unit has already moved this turn!"));
		return;
	}

	TArray<ATile*> HighlightedMovementTilesBeta;

	// Get and highlight movement tiles
	HighlightedMovementTilesBeta = SelectedUnit->GetMovementTiles();

	for (int32 i = 0; i <= HighlightedMovementTilesBeta.Num() - 1; i++)
	{
		ATile* Tile = HighlightedMovementTilesBeta[i];
		if (!Tile->IsObstacle())
		{
			HighlightedMovementTiles.Add(Tile);
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Found %d movement tiles"), HighlightedMovementTiles.Num()));

	// Apply the highlight material to each tile
	for (ATile* Tile : HighlightedMovementTiles)
	{
		if (Tile)
		{
			Tile->SetHighlightForMovement();
		}

		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan,
			FString::Printf(TEXT("Move Tile: (%f,%f)"),
				Tile->GetGridPosition().X,
				Tile->GetGridPosition().Y));
	}

	CurrentAction = EPlayerAction::MOVEMENT;
}

void ATBS_HumanPlayer::HighlightAttackTiles()
{
	// Clear any previous highlights
	ClearHighlightedTiles();

	// Ensure a unit is selected and can attack
	if (!SelectedUnit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Cannot highlight attack tiles. No unit selected."));
		return;
	}

	if (SelectedUnit->HasAttacked())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("This unit has already attacked this turn!"));
		return;
	}

	// Debug selected unit info
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Selected Unit: %s, Owner: %d, Location: (%f,%f)"),
			*SelectedUnit->GetUnitName(),
			SelectedUnit->GetOwnerID(),
			SelectedUnit->GetCurrentTile()->GetGridPosition().X,
			SelectedUnit->GetCurrentTile()->GetGridPosition().Y));

	// Get and highlight attack tiles
	HighlightedAttackTiles = SelectedUnit->GetAttackTiles();

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
		FString::Printf(TEXT("Found %d attack tiles"), HighlightedAttackTiles.Num()));

	// Apply the highlight material to each tile
	for (ATile* Tile : HighlightedAttackTiles)
	{
		if (Tile)
		{
			Tile->SetHighlightForAttack();
		}
	}

	CurrentAction = EPlayerAction::ATTACK;
}

void ATBS_HumanPlayer::ClearHighlightedTiles()
{
	// Clear visual highlights from all tiles
	for (ATile* Tile : HighlightedMovementTiles)
	{
		if (Tile)
		{
			Tile->ClearHighlight();
		}
	}

	for (ATile* Tile : HighlightedAttackTiles)
	{
		if (Tile)
		{
			Tile->ClearHighlight();
		}
	}

	// Clear arrays
	HighlightedMovementTiles.Empty();
	HighlightedAttackTiles.Empty();

	// Reset mode
	CurrentAction = EPlayerAction::NONE;
}

FString ATBS_HumanPlayer::GetSelectionMessage() const
{
	// Handle different scenarios
	if (!IsMyTurn)
	{
		return FString(TEXT("Not your turn"));
	}

	if (SelectedUnit)
	{
		// We have a selected unit - provide information about it
		FString UnitInfo = FString::Printf(TEXT("Selected: %s \nHealth: %d/%d"),
			*SelectedUnit->GetUnitName(),
			SelectedUnit->GetUnitHealth(),
			SelectedUnit->GetMaxHealth());

		// Add info about movement and attack availability
		if (SelectedUnit->HasMoved() && SelectedUnit->HasAttacked())
		{
			UnitInfo += TEXT(" \n| No actions available");
		}
		else
		{
			if (!SelectedUnit->HasMoved())
				UnitInfo += TEXT(" \n| Can move");

			if (!SelectedUnit->HasAttacked())
				UnitInfo += TEXT(" \n| Can attack");
		}

		return UnitInfo;
	}
	else if (CurrentAction == EPlayerAction::MOVEMENT)
	{
		return FString(TEXT("Select a tile to move to"));
	}
	else if (CurrentAction == EPlayerAction::ATTACK)
	{
		return FString(TEXT("Select an enemy to attack"));
	}
	else if (CurrentAction == EPlayerAction::PLACEMENT)
	{
		return FString(TEXT("Select a tile to place your unit"));
	}
	else
	{
		// Default state - no unit selected or enemy unit clicked
		return FString(TEXT("Click on your unit to select it"));
	}
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

void ATBS_HumanPlayer::SetCurrentPlacementTile(ATile* Tile)
{
	CurrentPlacementTile = Tile;
}

ATile* ATBS_HumanPlayer::GetCurrentPlacementTile()
{
	return CurrentPlacementTile;
}

void ATBS_HumanPlayer::ClearCurrentPlacementTile()
{
	CurrentPlacementTile = nullptr;
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