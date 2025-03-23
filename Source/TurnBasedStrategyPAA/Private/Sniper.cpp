// Fill out your copyright notice in the Description page of Project Settings.


#include "Sniper.h"
#include "Brawler.h"

ASniper::ASniper()
{
    // Sniper specific stats
    UnitType = EUnitType::SNIPER;
    UnitName = "Sniper";
    MovementRange = 3;
    AttackType = EAttackType::RANGE;
    AttackRange = 100;
    MinDamage = 40;
    MaxDamage = 80;
    Health = 20;
    MaxHealth = 20;

}

// Override of attack action to implement CounterAttack
int32 ASniper::Attack(AUnit* TargetUnit)
{
    if (!TargetUnit || bHasAttacked)
        return 0;

    // Checks if the target is within attack range
    TArray<ATile*> ValidAttackTiles = GetAttackTiles();
    if (!ValidAttackTiles.Contains(TargetUnit->GetCurrentTile()))
        return 0;

    // Calculates damage (random between min and max)
    int32 Damage = FMath::RandRange(MinDamage, MaxDamage);

    // Applies damage to target
    TargetUnit->ReceiveDamage(Damage);

    // Checks if the Sniper should receive self-damage based on what it attacked
    bool bShouldReceiveDamage = false;

    // Checks if target is another Sniper
    if (Cast<ASniper>(TargetUnit))
    {
        bShouldReceiveDamage = true;
    }
    // Checks if target is a Brawler within distance 1
    else if (Cast<ABrawler>(TargetUnit))
    {
        // Calculate distance between tiles
        FVector2D TargetPos = TargetUnit->GetCurrentTile()->GetGridPosition();
        FVector2D MyPos = CurrentTile->GetGridPosition();
        float Distance = FMath::Abs(TargetPos.X - MyPos.X) + FMath::Abs(TargetPos.Y - MyPos.Y);

        // Checks if distance is 1
        if (Distance <= 1)
        {
            bShouldReceiveDamage = true;
        }
    }

    // Applies counterattack damage if conditions are met
    if (bShouldReceiveDamage && !IsDead())
    {
        int32 SelfDamage = FMath::RandRange(1, 3);
        ReceiveDamage(SelfDamage);
    }

    bHasAttacked = true;
    return Damage;
}