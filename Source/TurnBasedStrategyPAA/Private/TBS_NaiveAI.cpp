// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_NaiveAI.h"
#include "Kismet/GameplayStatics.h"
#include "Grid.h"
#include "TBS_GameMode.h"
#include "TBS_GameInstance.h"
#include "EngineUtils.h"
#include "Sniper.h"
#include "Brawler.h"

// Sets default values
ATBS_NaiveAI::ATBS_NaiveAI()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a root component (similar to TBS_HumanPlayer's Camera)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Default values
	MinActionDelay = 0.5f;
	MaxActionDelay = 2.0f;
	bIsProcessingTurn = false;
	PlayerNumber = 1; // Default AI is player 1
	UnitColor = EUnitColor::RED;
	CurrentAction = EAIAction::NONE;
	SelectedUnit = nullptr;
}

// Called when the game starts or when spawned
void ATBS_NaiveAI::BeginPlay()
{
	Super::BeginPlay();

	// Get the game instance
	GameInstance = Cast<UTBS_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	// Find the grid
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		Grid = Cast<AGrid>(FoundActors[0]);
	}
}

// Called every frame
void ATBS_NaiveAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATBS_NaiveAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATBS_NaiveAI::OnTurn_Implementation()
{
	// First, explicitly check if it's actually our turn
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI: GameMode is null"));
		return;
	}

	// Early exit if it's not our turn
	if (GameMode->CurrentPlayer != PlayerNumber)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			FString::Printf(TEXT("AI OnTurn called but CurrentPlayer=%d, AI PlayerNumber=%d"),
				GameMode->CurrentPlayer, PlayerNumber));
		bIsProcessingTurn = false;
		return;
	}

	// Only here it's the AI's turn
	bIsProcessingTurn = true;

	GameInstance->SetTurnMessage(TEXT("AI (Random) Turn"));

	// Add slight delay before taking action to make it more natural
	bIsProcessingTurn = true;
	CurrentAction = EAIAction::NONE;

	// Find units at the start of turn
	FindMyUnits();

	float Delay = FMath::RandRange(MinActionDelay, MaxActionDelay);
	GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &ATBS_NaiveAI::ProcessTurnAction, Delay, false);
}

void ATBS_NaiveAI::SetTurnState_Implementation(bool bNewTurnState)
{
	// Safely set the turn state
	bIsProcessingTurn = bNewTurnState;

	// Use Execute_ method to call the UI update
	if (bNewTurnState)
	{
		ITBS_PlayerInterface::Execute_UpdateUI(this);
	}
}

void ATBS_NaiveAI::UpdateUI_Implementation()
{
	// Update any UI elements related to AI
	GameInstance = Cast<UTBS_GameInstance>(GetGameInstance());
	if (GameInstance && bIsProcessingTurn)
	{
		FString Message = TEXT("AI's Turn");

		// If there's a winner, append it to the message
		if (GameInstance->HasWinner())
		{
			Message = Message + TEXT(" | ") + GameInstance->GetWinnerMessage();
		}

		GameInstance->SetTurnMessage(Message);
	}
}

void ATBS_NaiveAI::OnWin_Implementation()
{
	if (GameInstance)
	{
		// Set the winner to the AI player (index 1)
		GameInstance->SetWinner(1);

		GameInstance->SetTurnMessage(TEXT("AI (Random) Wins!"));
		GameInstance->RecordGameResult(false); // Human didn't win
	}
}

void ATBS_NaiveAI::OnLose_Implementation()
{
	GameInstance->SetTurnMessage(TEXT("AI Loses!"));
}

void ATBS_NaiveAI::OnPlacement_Implementation()
{
	GameInstance->SetTurnMessage(TEXT("AI Placing Units"));

	// Add slight delay before placing units
	bIsProcessingTurn = true;
	CurrentAction = EAIAction::PLACEMENT;
	float Delay = FMath::RandRange(MinActionDelay, MaxActionDelay);
	GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &ATBS_NaiveAI::ProcessPlacementAction, Delay, false);
}

void ATBS_NaiveAI::FindMyUnits()
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

void ATBS_NaiveAI::ProcessPlacementAction()
{
	// Get the game mode
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI: GameMode is null"));
		return;
	}

	// Verify it's AI's turn
	if (GameMode->CurrentPlayer != PlayerNumber)
	{
		return;
	}

	// Track available unit types
	TArray<EUnitType> AvailableTypes;

	if (!GameMode->BrawlerPlaced[PlayerNumber])
	{
		AvailableTypes.Add(EUnitType::BRAWLER);
	}

	if (!GameMode->SniperPlaced[PlayerNumber])
	{
		AvailableTypes.Add(EUnitType::SNIPER);
	}

	// If no unit types are available, end processing
	if (AvailableTypes.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI Placement - No available unit types"));
		CurrentAction = EAIAction::NONE;
		bIsProcessingTurn = false;
		return;
	}

	// Try to place a unit with multiple attempts -> to avoid getting stuck in waiting
	int32 MaxPlacementAttempts = 5;
	bool Success = false;

	for (int32 Attempt = 0; Attempt < MaxPlacementAttempts && !Success; Attempt++)
	{
		// Randomly select from available unit types
		int32 RandomIndex = FMath::RandRange(0, AvailableTypes.Num() - 1);
		EUnitType TypeToPlace = AvailableTypes[RandomIndex];

		// Find random empty tile to place the unit
		int32 GridX, GridY;
		if (PickRandomTileForPlacement(GridX, GridY))
		{
			// Try to place the unit
			Success = GameMode->PlaceUnit(TypeToPlace, GridX, GridY, PlayerNumber);

			if (Success)
			{
				// Record move in game instance
				FString UnitTypeString = (TypeToPlace == EUnitType::BRAWLER) ? "Brawler" : "Sniper";
				if (GameInstance)
				{
					GameInstance->AddMoveToHistory(PlayerNumber, UnitTypeString, "Place",
						FVector2D::ZeroVector, FVector2D(GridX, GridY), 0);
				}

				break;
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("AI Placement - Could not find empty tile for attempt"));
		}
	}

	// If we still failed after all attempts, we need to handle this gracefully
	if (!Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("AI Placement - Failed after multiple attempts! Emergency handling..."));

		// Find ANY valid tile on the grid
		for (ATile* Tile : Grid->TileArray)
		{
			if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY && Tile->GetOwner() != -2)
			{
				FVector2D GridPos = Tile->GetGridPosition();
				int32 GridX = GridPos.X;
				int32 GridY = GridPos.Y;

				// Try placing with the first available unit type
				EUnitType TypeToPlace = AvailableTypes[0];
				Success = GameMode->PlaceUnit(TypeToPlace, GridX, GridY, PlayerNumber);

				if (Success)
				{
					FString UnitTypeString = (TypeToPlace == EUnitType::BRAWLER) ? "Brawler" : "Sniper";
					if (GameInstance)
					{
						GameInstance->AddMoveToHistory(PlayerNumber, UnitTypeString, "Place",
							FVector2D::ZeroVector, FVector2D(GridX, GridY), 0);
					}

					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
						TEXT("AI Placement - Emergency placement successful!"));
					break;
				}
			}
		}
	}

	// Even if we couldn't place a unit, we need to ensure the game continues
	if (!Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("WARNING: AI could not place unit! Marking as placed to avoid game freeze."));

		// Mark a unit as placed to prevent game freeze (choose the first available type)
		if (AvailableTypes.Contains(EUnitType::BRAWLER))
			GameMode->BrawlerPlaced[PlayerNumber] = true;
		else if (AvailableTypes.Contains(EUnitType::SNIPER))
			GameMode->SniperPlaced[PlayerNumber] = true;

		// Increment units placed to ensure game progresses
		GameMode->UnitsPlaced++;
	}

	// Reset action state
	CurrentAction = EAIAction::NONE;
	bIsProcessingTurn = false;
}

bool ATBS_NaiveAI::PickRandomTileForPlacement(int32& OutX, int32& OutY)
{
	if (!Grid)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI - Grid is null in PickRandomTileForPlacement"));
		return false;
	}

	// Create a list of all empty tiles with additional verification
	TArray<ATile*> EmptyTiles;
	int32 totalTiles = 0;
	int32 occupiedTiles = 0;
	int32 obstacleTiles = 0;
	int32 emptyTiles = 0;

	for (ATile* Tile : Grid->TileArray)
	{
		totalTiles++;

		if (!Tile)
			continue;

		ETileStatus status = Tile->GetTileStatus();
		int32 owner = Tile->GetOwner();

		if (status == ETileStatus::EMPTY)
		{
			// Double-check it's truly empty
			if (!Tile->GetOccupyingUnit() && owner != -2) // -2 is for obstacles
			{
				EmptyTiles.Add(Tile);
				emptyTiles++;
			}
			else if (owner == -2)
			{
				obstacleTiles++;
			}
		}
		else
		{
			occupiedTiles++;
		}
	}

	// If we found empty tiles, pick a random one
	if (EmptyTiles.Num() > 0)
	{
		int32 MaxAttempts = 10; // Prevent infinite loop
		for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
		{
			int32 RandomIndex = FMath::RandRange(0, EmptyTiles.Num() - 1);
			ATile* SelectedTile = EmptyTiles[RandomIndex];

			// Final verification that tile is truly empty
			if (SelectedTile && SelectedTile->GetTileStatus() == ETileStatus::EMPTY &&
				!SelectedTile->GetOccupyingUnit() && SelectedTile->GetOwner() != -2)
			{
				FVector2D GridPos = SelectedTile->GetGridPosition();
				OutX = GridPos.X;
				OutY = GridPos.Y;

				return true;
			}
			else
			{
				// Remove this tile from consideration and try again
				EmptyTiles.RemoveAt(RandomIndex);

				if (EmptyTiles.Num() == 0)
					break;
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI - Could not find any suitable empty tiles after verification"));
	return false;
}

void ATBS_NaiveAI::ProcessTurnAction()
{
	// Find all units owned by this AI
	FindMyUnits();

	// If no units, end turn
	if (MyUnits.Num() == 0)
	{
		FinishTurn();
		return;
	}

	// Process each unit in random order
	TArray<AUnit*> UnitsToProcess = MyUnits;
	while (UnitsToProcess.Num() > 0)
	{
		// Picks a random unit to process
		int32 RandomIndex = FMath::RandRange(0, UnitsToProcess.Num() - 1);
		AUnit* UnitToProcess = UnitsToProcess[RandomIndex];

		// Process the unit's action
		ProcessUnitAction(UnitToProcess);

		// Remove this unit from the list
		UnitsToProcess.RemoveAt(RandomIndex);
	}

	// End the turn after processing all units
	FinishTurn();
}

void ATBS_NaiveAI::ProcessUnitAction(AUnit* Unit)
{
	// Find all units owned by this AI
	FindMyUnits();

	if (!Unit || Unit->IsDead())
		return;

	// First, try to attack (50% chance)
	bool HasAttacked = false;
	if (FMath::RandBool())
	{
		HasAttacked = TryAttackWithUnit(Unit);
	}

	// Then, try to move (if not already moved)
	bool HasMoved = false;
	if (!Unit->HasMoved())
	{
		HasMoved = TryMoveUnit(Unit);
	}

	// If we moved but didn't attack yet, try to attack now
	if (HasMoved && !HasAttacked && !Unit->HasAttacked())
	{
		TryAttackWithUnit(Unit);
	}

	// If the unit cannot move or attack, skip its turn
	if (!Unit->HasMoved() && !Unit->HasAttacked())
	{
		Unit->bHasMoved = true;
		Unit->bHasAttacked = true;

		// Record skipped turn
		GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Skip", FVector2D::ZeroVector, FVector2D::ZeroVector, 0);
	}
}

bool ATBS_NaiveAI::TryMoveUnit(AUnit* Unit)
{
	if (!Unit || Unit->HasMoved())
		return false;

	// Get possible movement tiles
	TArray<ATile*> MovementTiles = Unit->GetMovementTiles();

	// If no tiles to move to, return false
	if (MovementTiles.Num() == 0)
		return false;

	// Pick a random tile to move to
	int32 RandomIndex = FMath::RandRange(0, MovementTiles.Num() - 1);
	ATile* TargetTile = MovementTiles[RandomIndex];

	// Record the initial position
	FVector2D FromPosition = Unit->GetCurrentTile()->GetGridPosition();
	FVector2D ToPosition = TargetTile->GetGridPosition();

	// Move the unit
	bool Success = Unit->MoveToTile(TargetTile);

	if (Success)
	{
		// Record move
		GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Move", FromPosition, ToPosition, 0);

		// Notify using game mode
		ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Move", FromPosition, ToPosition, 0);
		}
	}

	return Success;
}

bool ATBS_NaiveAI::TryAttackWithUnit(AUnit* Unit)
{
	if (!Unit || Unit->HasAttacked())
		return false;

	// Get possible attack tiles
	TArray<ATile*> AttackTiles = Unit->GetAttackTiles();

	// If no tiles to attack, return false
	if (AttackTiles.Num() == 0)
		return false;

	// Pick a random tile to attack
	int32 RandomIndex = FMath::RandRange(0, AttackTiles.Num() - 1);
	ATile* TargetTile = AttackTiles[RandomIndex];

	// Get the unit on the target tile
	AUnit* TargetUnit = TargetTile->GetOccupyingUnit();
	if (!TargetUnit)
		return false;

	// Get positions for recording
	FVector2D FromPosition = Unit->GetCurrentTile()->GetGridPosition();
	FVector2D ToPosition = TargetTile->GetGridPosition();

	// Attack the unit
	int32 Damage = Unit->Attack(TargetUnit);

	// Record attack
	if (GameInstance)
	{
		GameInstance->AddMoveToHistory(PlayerNumber, Unit->GetUnitName(), "Attack", FromPosition, ToPosition, Damage);
	}

	// Notify using game mode
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());

	GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Attack", FromPosition, ToPosition, Damage);

	return true;

}

void ATBS_NaiveAI::SkipUnitTurn()
{
	if (SelectedUnit)
	{
		// Mark the unit as having moved and attacked
		SelectedUnit->bHasMoved = true;
		SelectedUnit->bHasAttacked = true;

		// Record skipped turn in move history
		if (GameInstance)
		{
			GameInstance->AddMoveToHistory(PlayerNumber, SelectedUnit->GetUnitName(), "Skip", FVector2D::ZeroVector, FVector2D::ZeroVector, 0);
		}

		// Clear the selected unit
		SelectedUnit = nullptr;
	}
}

void ATBS_NaiveAI::ResetActionState()
{
	CurrentAction = EAIAction::NONE;
}

void ATBS_NaiveAI::EndTurn()
{
	// End our turn via the GameMode 
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->EndTurn();
	}

	// Reset turn-related states
	bIsProcessingTurn = false;
	CurrentAction = EAIAction::NONE;
	SelectedUnit = nullptr;
}

void ATBS_NaiveAI::FinishTurn()
{
	// End our turn via the GameMode
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());

	GameMode->EndTurn();

	bIsProcessingTurn = false;
}