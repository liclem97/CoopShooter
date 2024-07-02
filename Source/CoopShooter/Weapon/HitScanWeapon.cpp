// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "CoopShooter/AI/TrackerBot.h"
#include "CoopShooter/Character/CoopCharacterBase.h"
#include "CoopShooter/CoopComponents/LagCompensationComponent.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		ACoopCharacterBase* CoopCharacter = Cast<ACoopCharacterBase>(FireHit.GetActor());
		if (CoopCharacter && InstigatorController)
		{
			if (CoopCharacter)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

					UGameplayStatics::ApplyDamage(
						CoopCharacter,
						DamageToCause,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
				else if (!HasAuthority() && bUseServerSideRewind)
				{
					CoopOwnerCharacter = CoopOwnerCharacter == nullptr ? Cast<ACoopCharacterBase>(OwnerPawn) : CoopOwnerCharacter;
					CoopOwnerController = CoopOwnerController == nullptr ? Cast<ACoopPlayerController>(InstigatorController) : CoopOwnerController;
					if (CoopOwnerCharacter && CoopOwnerController && CoopOwnerCharacter->GetLagCompensation() && CoopOwnerCharacter->IsLocallyControlled())
					{
						CoopOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
							CoopCharacter,
							Start,
							HitTarget,
							CoopOwnerController->GetServerTime() - CoopOwnerController->SingleTripTime
						);
					}
				}
			}
		}
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25;
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}