// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.h"
#include "Grid.h"
#include "Unit.generated.h"

// Unit types
UENUM(BlueprintType)
enum class EUnitType : uint8
{
    NONE    UMETA(DisplayName = "None"),
    BRAWLER   UMETA(DisplayName = "Brawler"),
    SNIPER   UMETA(DisplayName = "Sniper"),
};

// Unit attack types
UENUM(BlueprintType)
enum class EAttackType : uint8
{
    NONE    UMETA(DisplayName = "None"),
    RANGE   UMETA(DisplayName = "Range"),
    MELEE   UMETA(DisplayName = "Melee"),
};

// Team color
UENUM(BlueprintType)
enum class EUnitColor : uint8
{
    NONE    UMETA(DisplayName = "None"),
    RED     UMETA(DisplayName = "Red"),
    BLUE    UMETA(DisplayName = "Blue"),
};

class AGrid;

UCLASS()
class TURNBASEDSTRATEGYPAA_API AUnit : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AUnit();

    // Set the unit's owner
    void SetOwnerID(int32 InOwnerID);

    // Get unit's name
    UFUNCTION(BlueprintCallable, Category = "Unit")
    FString GetUnitName() const;

    // Get the unit's owner
    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetOwnerID() const;

    // Get unit's Hp
    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetUnitHealth() const;

    // Get unit's Max Hp
    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetMaxHealth() const;

    // Get unit's type
    UFUNCTION(BlueprintCallable, Category = "Unit")
    EUnitType GetUnitType() const;

    // Get unit's Attack type
    UFUNCTION(BlueprintCallable, Category = "Unit")
    EAttackType GetAttackType() const;

    // Get unit's Attack Range
    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetAttackRange() const;

    // Get unit's min and max damage
    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetMinDamage() const;

    UFUNCTION(BlueprintCallable, Category = "Unit")
    int32 GetMaxDamage() const;

    // Get unit's average Attack Damage
    UFUNCTION(BlueprintCallable, Category = "Unit")
    float GetAverageAttackDamage() const;

    // Get unit's health/max health
    UFUNCTION(BlueprintCallable, Category = "Unit")
    FString GetLiveHealth();

    // Get Health's percentage
    UFUNCTION(BlueprintCallable, Category = "Unit")
    float GethealthPercentage();


    // Initialize unit's grid position
    UFUNCTION(BlueprintCallable, Category = "Unit")
    void InitializePosition(ATile* Tile);

    // Move the unit to a new tile
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool MoveToTile(ATile* Tile);

    // Highlight the reachable tiles
    UFUNCTION(BlueprintCallable, Category = "Unit")
    TArray<ATile*> GetMovementTiles();

    // Highlight the tiles within attack range
    UFUNCTION(BlueprintCallable, Category = "Unit")
    TArray<ATile*> GetAttackTiles();

    // Attack action
    UFUNCTION(BlueprintCallable, Category = "Unit")
    virtual int32 Attack(AUnit* TargetUnit);

    // Damage received
    UFUNCTION(BlueprintCallable, Category = "Unit")
    void ReceiveDamage(int32 DamageAmount);

    // Boolean to check if the unit is dead
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool IsDead() const;

    // Resets the unit's TURN (called at the start of a new turn. Only clears movement and attacks)
    UFUNCTION(BlueprintCallable, Category = "Unit")
    void ResetTurn();

    // Get the current tile
    UFUNCTION(BlueprintCallable, Category = "Unit")
    ATile* GetCurrentTile() const;

    // True if unit has moved this turn
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool HasMoved() const;

    // True if unit has attacked this turn
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool HasAttacked() const;

    // Has moved this turn
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bHasMoved;

    // Has attacked this turn
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bHasAttacked;

    UFUNCTION(BlueprintCallable, Category = "Unit Appearance")
    virtual void UpdateAppearanceByTeam();


protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // To add visuals to the scene
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneComponent;

    // Static mesh for the unit
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StaticMeshComponent;

    // String containing the name of a unit
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    FString UnitName;

    // Movement range
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 MovementRange;

    // Unit type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    EUnitType UnitType;

    // Attack type (range or melee)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    EAttackType AttackType;

    // Attack range
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 AttackRange;

    // Minimum damage
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 MinDamage;

    // Maximum damage
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 MaxDamage;

    // Health points
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 Health;

    // Max health points
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 MaxHealth;

    // Unit owner (0 for player, 1 for AI)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Properties")
    int32 OwnerID;

    // Current tile position
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    ATile* CurrentTile;

    // Reference to the grid
    UPROPERTY(Transient)
    AGrid* Grid;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

};