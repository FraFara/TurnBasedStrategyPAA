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
}

void ATBS_HumanPlayer::OnWin_Implementation()
{

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
}

void ATBS_HumanPlayer::OnPlacement_Implementation()
{
	// Check if this is player turn (match with game mode's current player)
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode && GameMode->CurrentPlayer != PlayerNumber)
	{
		// It's not player turn - don't do anything
		IsMyTurn = false;
		return;
	}

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
		GameMode->ShowUnitSelectionUI(true);
	}
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
	// Get the game mode for reference
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !GameMode->GameGrid)
	{
		return;
	}

	// Get hit result under cursor
	FHitResult Hit;
	bool bHitSuccessful = GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(
		ECollisionChannel::ECC_Visibility, false, Hit);

	if (!bHitSuccessful)
	{
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
		return;
	}

	// EXPLICIT CHECK: If in GAMEPLAY but CurrentAction is PLACEMENT, reset it
	if (GameMode->CurrentPhase == EGamePhase::GAMEPLAY && CurrentAction == EPlayerAction::PLACEMENT)
	{
		CurrentAction = EPlayerAction::NONE;
	}

	// Handle Setup/Placement Phase
	if (GameMode->CurrentPhase == EGamePhase::SETUP)
	{
		// Validate turn readiness
		if (!IsMyTurn)
		{
			return;
		}

		// Check tile status
		if (ClickedTile->GetTileStatus() != ETileStatus::EMPTY)
		{
			return;
		}

		// Set the current placement tile
		CurrentPlacementTile = ClickedTile;

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
			return;
		}

		// Get the unit on the tile
		AUnit* UnitOnTile = ClickedTile->GetOccupyingUnit();

		// Gameplay action logic
		switch (CurrentAction)
		{
		case EPlayerAction::NONE:
			// Unit selection logic
			if (UnitOnTile)
			{
				// Always update LastClickedUnit for HUD display
				LastClickedUnit = UnitOnTile;

				// Unit selection logic - ensure ownership check is correct
				if (UnitOnTile->GetOwnerID() == PlayerNumber)
				{
					// Update selected unit
					if (SelectedUnit == UnitOnTile)
					{
						// Deselect if already selected
						SelectedUnit = nullptr;
					}
					else
					{
						SelectedUnit = UnitOnTile;
					}
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
						// Record the move through the GameMode
						ATBS_GameMode* GameModeRef = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
						if (GameModeRef)
						{
							GameModeRef->RecordMove(PlayerNumber,
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

				if (Damage > 0)
				{
					// Record attack only through game mode
					ATBS_GameMode* GameModeRef = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
					if (GameModeRef)
					{
						GameModeRef->RecordMove(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Attack"), FromPos, TargetPos, Damage);
					}

					if (GameInstance)
					{
						GameInstance->SetTurnMessage(
							FString::Printf(TEXT("%s dealt %d damage to %s"),
								*SelectedUnit->GetUnitName(), Damage, *UnitOnTile->GetUnitName()));
					}
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

void ATBS_HumanPlayer::OnRightClick()
{
	// Cancel the current action
	if (CurrentAction != EPlayerAction::NONE)
	{
		// Clear highlighted tiles
		ClearHighlightedTiles();

		// Reset action
		CurrentAction = EPlayerAction::NONE;
	}
	// If no action is active but a unit is selected, deselect it
	else if (SelectedUnit)
	{
		SelectedUnit = nullptr;

		ClearHighlightedTiles();
		
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

	// Get the game mode to check if the current player matches
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());

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
}


void ATBS_HumanPlayer::EndTurn()
{
}

void ATBS_HumanPlayer::HighlightMovementTiles()
{
	// Clear any previous highlights
	ClearHighlightedTiles();

	if (!SelectedUnit)
	{
		return;
	}

	if (SelectedUnit->HasMoved())
	{
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

	// Apply the highlight material to each tile
	for (ATile* Tile : HighlightedMovementTiles)
	{
		if (Tile)
		{
			Tile->SetHighlightForMovement();
		}

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
		return;
	}

	if (SelectedUnit->HasAttacked())
	{
		return;
	}

	// Get and highlight attack tiles
	HighlightedAttackTiles = SelectedUnit->GetAttackTiles();

	// Check if there are no attackable tiles
	if (HighlightedAttackTiles.Num() == 0)
	{
		// Update the turn message to inform the player
		if (GameInstance)
		{
			FString CurrentMessage = GameInstance->GetTurnMessage();
			GameInstance->SetTurnMessage(CurrentMessage + TEXT(" - No units within attack range"));

			// Set a timer to revert the message after a few seconds
			FTimerHandle MessageTimerHandle;
			GetWorldTimerManager().SetTimer(MessageTimerHandle, [this]()
				{
					if (GameInstance)
					{
						// Reset to basic turn message
						GameInstance->SetTurnMessage(TEXT("Your Turn - Select a unit"));
					}
				}, 3.0f, false);
		}

		// Don't set attack mode if there are no targets
		CurrentAction = EPlayerAction::NONE;
		return;
	}

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