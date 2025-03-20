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
	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

	// Initialize UnitToPlace to NONE
	UnitToPlace = EUnitType::NONE;

	// Log that the human player is ready
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Human Player Initialized"));

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
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Your Turn - IsMyTurn set to %d"), IsMyTurn ? 1 : 0));

	// Find all your units at the start of your turn
	FindMyUnits();

	// Additional debug
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Found %d units for player"), MyUnits.Num()));

	// Debug each unit found
	for (AUnit* Unit : MyUnits)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Unit: %s at (%f,%f)"), *Unit->GetUnitName(), Unit->GetCurrentTile()->GetGridPosition().X, Unit->GetCurrentTile()->GetGridPosition().Y));
	}
}

void ATBS_HumanPlayer::OnWin_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("You Win!"));
}

void ATBS_HumanPlayer::OnLose_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("You Lose!"));
}

void ATBS_HumanPlayer::OnPlacement_Implementation()
{
	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	IsMyTurn = true;
	CurrentAction = EPlayerAction::PLACEMENT;

	// Set message
	GameInstance->SetTurnMessage(TEXT("Place Your Units"));
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Calling ShowUnitSelectionUI from Human Player"));
		GameMode->ShowUnitSelectionUI();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("GameMode is null from Human Player"));
	}

	// Single debug message without repeat
	static bool bFirstPlacement = true;
	if (bFirstPlacement)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Your Turn to Place Units"));
		bFirstPlacement = false;
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
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GameMode is null in OnClick"));
		return;
	}

	// Debug current game state
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
		FString::Printf(TEXT("OnClick - Phase: %d, IsMyTurn: %d, CurrentPlayer: %d, UnitToPlace: %d"),
			(int32)GameMode->CurrentPhase, IsMyTurn ? 1 : 0, GameMode->CurrentPlayer, (int32)UnitToPlace));

	// Early exit if it's not your turn
	if (GameMode->CurrentPlayer != PlayerNumber)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Cannot act - Not your turn"));
		return;
	}

	// Handle setup/placement phase
	if (GameMode->CurrentPhase == EGamePhase::SETUP)
	{
		// Ensure it's the setup phase and it's the human player's turn
		if (!IsMyTurn || UnitToPlace == EUnitType::NONE)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("Cannot place unit. Not your turn or no unit selected."));
			return;
		}

		// Get hit result under cursor
		FHitResult Hit = FHitResult(ForceInit);
		GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(
			ECollisionChannel::ECC_Pawn, true, Hit);

		// Check if something was hit
		if (Hit.bBlockingHit)
		{
			ATile* ClickedTile = Cast<ATile>(Hit.GetActor());
			if (ClickedTile)
			{
				// Get grid position of clicked tile
				FVector2D GridPos = ClickedTile->GetGridPosition();

				// Attempt to place the unit
				bool Success = GameMode->PlaceUnit(UnitToPlace, GridPos.X, GridPos.Y, PlayerNumber);

				if (Success)
				{
					// Reset unit to place after successful placement
					UnitToPlace = EUnitType::NONE;

					// Additional debug message
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
						TEXT("Unit placed successfully"));
				}
			}
		}
		return;
	}

	// Extra debug for gameplay phase
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
		FString::Printf(TEXT("Gameplay phase - IsMyTurn: %d, GameMode->CurrentPlayer: %d, PlayerNumber: %d"),
			IsMyTurn ? 1 : 0, GameMode->CurrentPlayer, PlayerNumber));

	// Existing gameplay phase logic
	if (GameMode->CurrentPhase != EGamePhase::GAMEPLAY || !IsMyTurn)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Cannot select unit. Not in gameplay phase or not your turn."));
		return;
	}

	// Get game instance for message updates
	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());

	// Get hit result under cursor
	FHitResult Hit = FHitResult(ForceInit);
	GetWorld()->GetFirstPlayerController()->GetHitResultUnderCursor(
		ECollisionChannel::ECC_Pawn, true, Hit);

	// Check if something was hit
	if (Hit.bBlockingHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Hit something: %s"), *Hit.GetActor()->GetName()));

		ATile* ClickedTile = Cast<ATile>(Hit.GetActor());
		if (ClickedTile)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
				FString::Printf(TEXT("Clicked on tile at (%f, %f)"),
					ClickedTile->GetGridPosition().X, ClickedTile->GetGridPosition().Y));

			AUnit* UnitOnTile = ClickedTile->GetOccupyingUnit();
			if (UnitOnTile)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
					FString::Printf(TEXT("Unit on tile: %s, Owner: %d"),
						*UnitOnTile->GetUnitName(), UnitOnTile->GetOwnerID()));
			}

			// Switch statement for gameplay actions
			switch (CurrentAction)
			{
			case EPlayerAction::NONE:
				// Unit selection logic
				if (UnitOnTile && UnitOnTile->GetOwnerID() == PlayerNumber)
				{
					// Debug unit info
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
						FString::Printf(TEXT("Clicked on unit - Type: %s, HP: %d/%d"),
							*UnitOnTile->GetUnitName(),
							UnitOnTile->GetUnitHealth(),
							UnitOnTile->GetMaxHealth()));

					// Update selected unit
					if (SelectedUnit == UnitOnTile)
					{
						// Deselect if already selected
						SelectedUnit = nullptr;
						GameInstance->SetTurnMessage(TEXT("Unit deselected"));
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Unit deselected"));
					}
					else
					{
						SelectedUnit = UnitOnTile;
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
							FString::Printf(TEXT("Unit selected: %s"), *UnitOnTile->GetUnitName()));

						// Prepare action message
						FString ActionMsg;
						if (!UnitOnTile->HasMoved())
							ActionMsg += TEXT("Movement available. ");
						if (!UnitOnTile->HasAttacked())
							ActionMsg += TEXT("Attack available. ");
						ActionMsg += TEXT("Unit can Skip Turn.");

						GameInstance->SetTurnMessage(ActionMsg);
					}
				}
				break;

			case EPlayerAction::MOVEMENT:
				// Movement logic
				if (SelectedUnit && !SelectedUnit->HasMoved() &&
					HighlightedMovementTiles.Contains(ClickedTile))
				{
					FVector2D FromPos = SelectedUnit->GetCurrentTile()->GetGridPosition();
					FVector2D ToPos = ClickedTile->GetGridPosition();

					if (SelectedUnit->MoveToTile(ClickedTile))
					{
						// Record move in history
						GameInstance->AddMoveToHistory(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Move"), FromPos, ToPos, 0);

						GameMode->RecordMove(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Move"), FromPos, ToPos, 0);

						ClearHighlightedTiles();

						// Check if unit can still attack
						if (!SelectedUnit->HasAttacked())
						{
							GameInstance->SetTurnMessage(TEXT("Attack available"));
						}
						else
						{
							SelectedUnit = nullptr;
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
						// Record attack in history
						GameInstance->AddMoveToHistory(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Attack"), FromPos, TargetPos, Damage);

						GameMode->RecordMove(PlayerNumber,
							SelectedUnit->GetUnitName(),
							TEXT("Attack"), FromPos, TargetPos, Damage);

						GameInstance->SetTurnMessage(
							FString::Printf(TEXT("%s dealt %d damage to %s"),
								*SelectedUnit->GetUnitName(), Damage, *UnitOnTile->GetUnitName()));
					}

					ClearHighlightedTiles();
					SelectedUnit = nullptr;
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

void ATBS_HumanPlayer::SelectBrawlerForPlacement()
{
	UnitToPlace = EUnitType::BRAWLER;
	CurrentAction = EPlayerAction::PLACEMENT;
	IsMyTurn = true;

	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	GameInstance->SetTurnMessage(TEXT("Place Brawler - Click on an empty tile"));
}

void ATBS_HumanPlayer::SelectSniperForPlacement()
{
	UnitToPlace = EUnitType::SNIPER;
	CurrentAction = EPlayerAction::PLACEMENT;
	IsMyTurn = true;

	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
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
	}
}

void ATBS_HumanPlayer::SetTurnState_Implementation(bool bNewTurnState)
{
	IsMyTurn = bNewTurnState;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
		FString::Printf(TEXT("Human Player turn state changed to: %d"), IsMyTurn ? 1 : 0));

	// Update UI to reflect turn state
	UpdateUI();
}

void ATBS_HumanPlayer::UpdateUI_Implementation()
{
	// This would connect to your existing UI without needing a separate HUD class
	// Update "Player's Turn" text in the UI
	UTBS_GameInstance* GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	if (GameInstance)
	{
		if (IsMyTurn)
		{
			GameInstance->SetTurnMessage(TEXT("Your Turn - Select a unit"));
		}
		else
		{
			GameInstance->SetTurnMessage(TEXT("AI's Turn"));
		}
	}

	// Additional UI updates can be added here
}

void ATBS_HumanPlayer::EndTurn()
{
}

//void ATBS_HumanPlayer::HighlightMovementTiles()
//{
//	if (!IsMyTurn || !SelectedUnit || SelectedUnit->HasMoved())
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot move this unit"));
//		return;
//	}
//
//	// Clear any existing highlights
//	ClearHighlightedTiles();
//
//	// Get movement tiles
//	HighlightedMovementTiles = SelectedUnit->GetMovementTiles();
//
//	// Apply visual highlight to each tile (this is where you'd add material changes)
//	for (ATile* Tile : HighlightedMovementTiles)
//	{
//		// Add visual highlight here - for now just debug msg
//		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green,
//			FString::Printf(TEXT("Movement tile: (%d, %d)"),
//				(int32)Tile->GetGridPosition().X, (int32)Tile->GetGridPosition().Y));
//	}
//
//	// Set action
//	CurrentAction = EPlayerAction::MOVEMENT;
//
//	// Highlight Tiles IMPLEMENTA https://www.youtube.com/watch?v=R7oLZL97XYo&ab_channel=BuildGamesWithJon
//}

void ATBS_HumanPlayer::HighlightMovementTiles()
{
	// More robust checking
	if (!IsMyTurn)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not your turn"));
		return;
	}

	if (!SelectedUnit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No unit selected"));
		return;
	}

	if (SelectedUnit->HasMoved())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unit has already moved"));
		return;
	}

	// Clear previous highlights
	ClearHighlightedTiles();

	// Get movement tiles
	HighlightedMovementTiles = SelectedUnit->GetMovementTiles();

	// Debug movement tiles
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Found %d movement tiles"), HighlightedMovementTiles.Num()));

	// Highlight tiles visually (add this code)
	for (ATile* Tile : HighlightedMovementTiles)
	{
		// Highlight tile - IMPLEMENT
		// This is placeholder logic - implement actual highlighting
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green,
			FString::Printf(TEXT("Movement tile: (%f, %f)"),
				Tile->GetGridPosition().X, Tile->GetGridPosition().Y));
	}

	// Set action
	CurrentAction = EPlayerAction::MOVEMENT;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		TEXT("Movement mode activated - Select a highlighted tile to move"));
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