// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopCharacterBase.h"
#include "CoopShooter/CoopComponents/CombatComponent.h"

ACoopCharacterBase::ACoopCharacterBase()
{
}

bool ACoopCharacterBase::IsFriendly(AActor* Attacker, AActor* Victim)
{
    if (Attacker == nullptr || Victim == nullptr) return false;
    if (Attacker == Victim) return false;

    ACoopCharacterBase* AttackerCharacter = Cast<ACoopCharacterBase>(Attacker);
    ACoopCharacterBase* VictimCharacter = Cast<ACoopCharacterBase>(Victim);

    if (AttackerCharacter == nullptr || VictimCharacter == nullptr) return false;
    

    return AttackerCharacter->GetTeamNum() == VictimCharacter->GetTeamNum();
}

AWeapon* ACoopCharacterBase::GetEquippedWeapon()
{
    if (Combat == nullptr) return nullptr;
    return Combat->EquippedWeapon;
}
