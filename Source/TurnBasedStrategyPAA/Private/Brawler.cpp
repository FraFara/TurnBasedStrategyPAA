#include "Brawler.h"

ABrawler::ABrawler()
{
    // Brawler stats
    MovementRange = 6;
    AttackType = EAttackType::MELEE;
    AttackRange = 1;
    MinDamage = 1;
    MaxDamage = 6;
    Health = 40;
    MaxHealth = 40;

}