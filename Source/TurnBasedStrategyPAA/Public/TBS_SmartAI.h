// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TBS_PlayerInterface.h"
#include "Unit.h"
#include "Tile.h"
#include "TBS_SmartAI.generated.h"

// Forward declarations
enum class EUnitColor : uint8;

// Enum for AI actions
UENUM(BlueprintType)
enum class ESAIAction : uint8
{
    NONE        UMETA(DisplayName = "None"),
    MOVEMENT    UMETA(DisplayName = "Movement"),
    ATTACK      UMETA(DisplayName = "Attack"),
    PLACEMENT   UMETA(DisplayName = "Unit Placement")
};

UCLASS()
class TURNBASEDSTRATEGYPAA_API ATBS_SmartAI : public APawn, public ITBS_PlayerInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    ATBS_SmartAI();

    // Reference to game instance
    UPROPERTY()
    class UTBS_GameInstance* GameInstance;

    // Player identification properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
    int32 PlayerNumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
    EUnitColor UnitColor;

    // Flag to track if AI is currently processing a turn
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game State")
    bool bIsProcessingTurn;

    // Current AI action
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
    ESAIAction CurrentAction;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Random delay between AI actions
    UPROPERTY(EditAnywhere, Category = "AI")
    float MinActionDelay;

    UPROPERTY(EditAnywhere, Category = "AI")
    float MaxActionDelay;

    // Handle for the timer that controls the AI's thinking time
    FTimerHandle ActionTimerHandle;

    // Reference to the grid
    UPROPERTY()
    class AGrid* Grid;

    // Current unit being controlled by AI
    UPROPERTY()
    AUnit* SelectedUnit;

    // Array of AI's units
    UPROPERTY()
    TArray<AUnit*> MyUnits;

    // Array of opponent's units
    UPROPERTY()
    TArray<AUnit*> EnemyUnits;

    // Current placement phase unit type
    EUnitType CurrentPlacementType;

    // Finds all units owned by this AI
    void FindMyUnits();

    // Finds all units owned by the opponent
    void FindEnemyUnits();

    // Executes a move for a single unit
    void ProcessUnitAction(AUnit* Unit);

    // A* Pathfinding implementation
    TArray<ATile*> FindPathAStar(ATile* StartTile, ATile* GoalTile);

    // Handle unit movement using A*
    bool TryMoveUnit(AUnit* Unit);

    // Handle unit attack with strategic target selection
    bool TryAttackWithUnit(AUnit* Unit);

    // Determine the best target for attack
    AUnit* SelectBestAttackTarget(AUnit* AttackingUnit, const TArray<ATile*>& AttackableTiles);

    // Determine the best movement destination for a unit
    ATile* SelectBestMovementDestination(AUnit* Unit);

    // Pick a random tile for unit placement (same as NaiveAI)
    bool PickRandomTileForPlacement(int32& OutX, int32& OutY);

    // AI action processing methods
    void ProcessPlacementAction();
    void ProcessTurnAction();
    void FinishTurn();

    // Helper struct for A* algorithm
    struct FAStarNode
    {
        ATile* Tile;
        float GScore; // Cost from start to this node
        float FScore; // GScore + HScore (estimated cost to goal)
        FAStarNode* Parent;

        FAStarNode() : Tile(nullptr), GScore(FLT_MAX), FScore(FLT_MAX), Parent(nullptr) {}
        FAStarNode(ATile* InTile) : Tile(InTile), GScore(FLT_MAX), FScore(FLT_MAX), Parent(nullptr) {}
    };

    // Manhattan distance
    float CalculateHeuristic(const FVector2D& Start, const FVector2D& Goal) const;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Interface implementations
    virtual void OnPlacement_Implementation() override;
    virtual void OnTurn_Implementation() override;
    virtual void OnWin_Implementation() override;
    virtual void OnLose_Implementation() override;
    virtual void SetTurnState_Implementation(bool bIsMyTurn) override;
    virtual void UpdateUI_Implementation() override;

    // Skip method
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    void SkipUnitTurn();

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    void ResetActionState();
};