// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/CoopComponents/BuffComponent.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(OtherActor);
	ACoopPlayerController* CoopPlayerController = Cast<ACoopPlayerController>(CoopCharacter->GetController());
	if (CoopCharacter && CoopPlayerController)
	{
		UBuffComponent* Buff = CoopCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	else return;
	Destroy();
}