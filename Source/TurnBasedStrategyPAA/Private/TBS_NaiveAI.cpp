// Fill out your copyright notice in the Description page of Project Settings.

#include "TBS_NaiveAI.h"
#include "Kismet/GameplayStatics.h"
#include "Grid.h"
#include "TBS_GameMode.h"
//#include "TBS_GameInstance.h"
#include "EngineUtils.h"

// Sets default values
ATBS_NaiveAI::ATBS_NaiveAI()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a root component (similar to TBS_HumanPlayer's Camera)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Default values
	//MinActionDelay = 0.5f;
	//MaxActionDelay = 2.0f;
	bIsProcessingTurn = false;
	PlayerNumber = 1; // Default AI is player 1
	UnitColor = EUnitColor::RED;
}

// Called when the game starts or when spawned
void ATBS_NaiveAI::BeginPlay()
{
	Super::BeginPlay();

	// Get the game instance (similar to TTT_RandomPlayer)
	GameInstance = Cast<UTBS_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	//// Find the grid
	//TArray<AActor*> FoundActors;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), FoundActors);
	//if (FoundActors.Num() > 0)
	//{
	//	Grid = Cast<AGrid>(FoundActors[0]);
	//}
}

// Called every frame
void ATBS_NaiveAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
//void ATBS_NaiveAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//	// AI doesn't need input bindings
//}

void ATBS_NaiveAI::OnTurn()
{
	// Similar to TTT_RandomPlayer's OnTurn
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI (Random) Turn"));

		//GameInstance->SetTurnMessage(TEXT("AI (Random) Turn"));

	// Add slight delay before taking action to make it more natural
	bIsProcessingTurn = true;
	float Delay = FMath::RandRange(MinActionDelay, MaxActionDelay);
	GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &ATBS_NaiveAI::ProcessTurnAction, Delay, false);
}

void ATBS_NaiveAI::OnWin()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI (Random) Wins!"));

		//GameInstance->SetTurnMessage(TEXT("AI (Random) Wins!"));
		//GameInstance->RecordGameResult(false); // Human didn't win
}

void ATBS_NaiveAI::OnLose()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI (Random) Loses!"));

		//GameInstance->SetTurnMessage(TEXT("AI Loses!"));
}

void ATBS_NaiveAI::OnPlacement()
{
	// AI received placement notification
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AI Placing Units"));

		//GameInstance->SetTurnMessage(TEXT("AI Placing Units"));

	// Add slight delay before placing units
	bIsProcessingTurn = true;
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
	// Randomly decide whether to place Brawler or Sniper
	EUnitType TypeToPlace;
	int32 RandomChoice = FMath::RandRange(0, 1);
	if (RandomChoice == 0)
	{
		TypeToPlace = EUnitType::BRAWLER;
	}
	else
	{
		TypeToPlace = EUnitType::SNIPER;
	}

	// Find random empty tile to place the unit
	int32 GridX, GridY;
	if (PickRandomTileForPlacement(GridX, GridY))
	{
		// Place the unit using the GameMode
		ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());

		GameMode->PlaceUnit(TypeToPlace, GridX, GridY, PlayerNumber);

		// Record move in game instance
		//FString UnitTypeString = (TypeToPlace == EUnitType::BRAWLER) ? "Brawler" : "Sniper";
		//GameInstance->AddMoveToHistory({
		//	PlayerNumber,
		//	UnitTypeString,
		//	"Place",
		//	FVector2D::ZeroVector,
		//	FVector2D(GridX, GridY),
		//	0
		//	});
	}
bIsProcessingTurn = false;
}

bool ATBS_NaiveAI::PickRandomTileForPlacement(int32& OutX, int32& OutY)
{

	// Create a list of all empty tiles
	TArray<ATile*> EmptyTiles;

	for (ATile* Tile : Grid->TileArray)
	{
		if (Tile && Tile->GetTileStatus() == ETileStatus::EMPTY)
		{
			EmptyTiles.Add(Tile);
		}
	}

	// If we found empty tiles, pick a random one
	if (EmptyTiles.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, EmptyTiles.Num() - 1);
		ATile* SelectedTile = EmptyTiles[RandomIndex];

		FVector2D GridPos = SelectedTile->GetGridPosition();
		OutX = GridPos.X;
		OutY = GridPos.Y;

		return true;
	}

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
			//GameInstance->AddMoveToHistory({
			//	PlayerNumber,
			//	Unit->GetUnitName(),
			//	"Skip",
			//	FVector2D::ZeroVector,
			//	FVector2D::ZeroVector,
			//	0
			//	});
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
		//GameInstance->AddMoveToHistory({
		//	PlayerNumber,
		//	Unit->GetUnitName(),
		//	"Move",
		//	FromPosition,
		//	ToPosition,
		//	0
		//	});

		//// Notify using game mode
		//ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
		//if (GameMode)
		//{
		//	GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Move", FromPosition, ToPosition, 0);
		//}
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

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("AI %s dealt %d damage to %s"), *Unit->GetUnitName(), Damage, *TargetUnit->GetUnitName()));

		// Record attack
		//if (GameInstance)
		//{
		//	GameInstance->AddMoveToHistory({
		//		PlayerNumber,
		//		Unit->GetUnitName(),
		//		"Attack",
		//		FromPosition,
		//		ToPosition,
		//		Damage
		//		});
		//}

		// Notify using game mode
		//ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
		//if (GameMode)
		//{
		//	GameMode->RecordMove(PlayerNumber, Unit->GetUnitName(), "Attack", FromPosition, ToPosition, Damage);
		//}

		return true;

}

void ATBS_NaiveAI::FinishTurn()
{
	// End our turn via the GameMode
	ATBS_GameMode* GameMode = Cast<ATBS_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->EndTurn();
	}

	bIsProcessingTurn = false;
}