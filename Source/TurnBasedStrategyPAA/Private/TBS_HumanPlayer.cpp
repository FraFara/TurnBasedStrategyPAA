// Fill out your copyright notice in the Description page of Project Settings.


#include "TBS_HumanPlayer.h"
#include "Grid.h"
#include "Sniper.h"
#include "Brawler.h"
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

void ATBS_HumanPlayer::OnPlacementTurn()
{
	IsMyTurn = true;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Place Your Units"));
}

void ATBS_HumanPlayer::OnClick()
{
	//Structure containing information about one hit of a trace, such as point of impact and surface normal at that point
	FHitResult Hit = FHitResult(ForceInit);
	// GetHitResultUnderCursor function sends a ray from the mouse position and gives the corresponding hit results
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

			// Handle based on current mode
			switch (CurrentAction)
			{
			case EPlayerAction::NONE:
				// If clicked a unit, show info and possibly select it
				if (UnitOnTile)
				{
					// Show unit info
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, *UnitOnTile->GetLiveHealth());

					// If it's our unit, select it
					if (UnitOnTile->GetOwnerID() == PlayerNumber)
					{
						// If already selected, deselect it
						if (SelectedUnit == UnitOnTile)
						{
							SelectedUnit = nullptr;
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Unit deselected"));
						}
						else
						{
							SelectedUnit = UnitOnTile;
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Selected %s"), *UnitOnTile->GetUnitName()));

							// Show available actions
							if (!UnitOnTile->HasMoved())
							{
								GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Movement avaiable"));
							}

							if (!UnitOnTile->HasAttacked())
							{
								GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Attack avaiable"));
							}

							if (!UnitOnTile->HasAttacked() || !UnitOnTile->HasMoved())
							{
								GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unit can Skip Turn"));
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
						// Move the unit
						SelectedUnit->MoveToTile(ClickedTile);
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Unit moved"));

						// Clear highlighted tiles
						ClearHighlightedTiles();

						// If the unit can still attack, show message
						if (!SelectedUnit->HasAttacked())
						{
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Press 'A' to attack"));
						}
						else
						{
							// Unit is done, deselect
							SelectedUnit = nullptr;
						}
					}
					else
					{
						// Invalid move target
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot move there"));
					}
				}
				break;

			case EPlayerAction::ATTACK:
				// If in attack mode, try to attack
				if (SelectedUnit && !SelectedUnit->HasAttacked())
				{
					if (HighlightedAttackTiles.Contains(ClickedTile) && UnitOnTile)
					{
						// Attack the unit
						int32 Damage = SelectedUnit->Attack(UnitOnTile);

						// Show attack result
						if (Damage > 0)
						{
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
								FString::Printf(TEXT("%s dealt %d damage to %s"),
									*SelectedUnit->GetUnitName(), Damage, *UnitOnTile->GetUnitName()));
						}

						// Clear highlighted tiles
						ClearHighlightedTiles();

						// Unit is done after attack, deselect
						SelectedUnit = nullptr;
					}
					else
					{
						// Invalid attack target
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot attack there"));
					}
				}
				break;

			case EPlayerAction::PLACEMENT:
				// Placement logic will be handled by GameMode
				if (ClickedTile->GetTileStatus() == ETileStatus::EMPTY)
				{
					// Place unit logic goes here - would be handled via GameMode
					// For now, just show a message
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
						FString::Printf(TEXT("Selected tile at (%d, %d) for placement"),
							(int32)ClickedTile->GetGridPosition().X, (int32)ClickedTile->GetGridPosition().Y));
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cannot place unit here"));
				}
				break;
			}
		}
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

	void ATBS_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, FString UnitName)
	{
		// Implementation would connect to GameMode's PlaceUnit function
		// Get the game mode
		AGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode();
		if (GameModeBase)
		{
			// Try to cast to our game mode
			ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GameModeBase);
			if (GameMode)
			{
				// Place the unit through the game mode
				GameMode->PlaceUnit(UnitName, GridX, GridY, PlayerNumber);
			}
		}
	}

	void ATBS_HumanPlayer::SkipUnitTurn()
	{
		if (!SelectedUnit)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No unit selected to skip"));
			return;
		}

		// Mark the unit as having moved and attacked
		SelectedUnit->bHasMoved = true;
		SelectedUnit->bHasAttacked = true;

		// Display a message
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
			FString::Printf(TEXT("Turn skipped"));

		// Clear the selection
		SelectedUnit = nullptr;

		// Clear any highlights
		ClearHighlightedTiles();

		// Check if all units have completed their turn
		TArray<AActor*> PlayerUnits;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), PlayerUnits);

		bool bAllUnitsDone = true;
		for (AActor* Actor : PlayerUnits)
		{
			AUnit* Unit = Cast<AUnit>(Actor);
			if (Unit && Unit->GetOwnerID() == PlayerNumber && (!Unit->HasMoved() || !Unit->HasAttacked()))
			{
				bAllUnitsDone = false;
				break;
			}
		}

		// If all units are done, end turn
		if (bAllUnitsDone)
		{
			EndTurn();
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


