// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "CoopShooter/Character/CoopCharacterBase.h"
#include "CoopShooter/CoopComponents/LagCompensationComponent.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{	
	AWeapon::Fire(FVector());
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		// ��Ʈ ĳ���͸� ��Ʈ Ƚ���� �����Ѵ�.
		TMap<ACoopCharacterBase*, uint32> HitMap;
		TMap<ACoopCharacterBase*, uint32> HeadShotHitMap;
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ACoopCharacterBase* CoopCharacter = Cast<ACoopCharacterBase>(FireHit.GetActor());
			if (CoopCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");
				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(CoopCharacter)) HeadShotHitMap[CoopCharacter]++;
					else HeadShotHitMap.Emplace(CoopCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(CoopCharacter)) HitMap[CoopCharacter]++;
					else HitMap.Emplace(CoopCharacter, 1);
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
						FireHit.ImpactPoint,
						.5f,
						FMath::FRandRange(-.5f, .5f)
					);
				}
			}
		}
		TArray<ACoopCharacterBase*> HitCharacters;

		// ĳ���� ��Ʈ total damage ��
		TMap<ACoopCharacterBase*, float> DamageMap;

		// �ٵ� ������ ���, hit * Damage ; DamageMap�� ����
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && InstigatorController)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

				HitCharacters.AddUnique(HitPair.Key);
			}
		}

		// ��弦 ������ ���, hit * HeadShotDamage ; DamageMap�� ����
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key && InstigatorController)
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) HeadShotHitMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		// DamageMap�� �ݺ��Ͽ� ĳ���Ϳ� ���� �� ���ط��� ������
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key, // ���� ĳ����
						DamagePair.Value, // ���� �ΰ��� �������� ���� �������� ��
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}


		if (!HasAuthority() && bUseServerSideRewind)
		{	
			CoopOwnerCharacter = CoopOwnerCharacter == nullptr ? Cast<ACoopCharacterBase>(OwnerPawn) : CoopOwnerCharacter;
			CoopOwnerController = CoopOwnerController == nullptr ? Cast<ACoopPlayerController>(InstigatorController) : CoopOwnerController;
			if (CoopOwnerCharacter && CoopOwnerController && CoopOwnerCharacter->GetLagCompensation() && CoopOwnerCharacter->IsLocallyControlled())
			{
				CoopOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					CoopOwnerController->GetServerTime() - CoopOwnerController->SingleTripTime
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
	}
}
