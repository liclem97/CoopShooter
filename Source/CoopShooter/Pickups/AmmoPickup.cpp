// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/CoopComponents/CombatComponent.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(OtherActor);
	ACoopPlayerController* CoopPlayerController = Cast<ACoopPlayerController>(CoopCharacter->GetController());
	if (CoopCharacter && CoopPlayerController)
	{
		UCombatComponent* Combat = CoopCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	else return;
	Destroy();
}