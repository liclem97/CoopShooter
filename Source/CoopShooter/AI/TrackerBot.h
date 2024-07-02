// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CoopShooter/Character/CoopCharacterBase.h"
#include "CoopShooter/CoopTypes/Team.h"
#include "TrackerBot.generated.h"

UCLASS()
class COOPSHOOTER_API ATrackerBot : public ACoopCharacterBase
{
	GENERATED_BODY()

public:
	ATrackerBot();
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class	USphereComponent* SphereComp;

	UPROPERTY(EditAnywhere, Category = "Components")
	UBoxComponent* HitBox;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UFloatingPawnMovement* FloatingPawnMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* AudioComp;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bUseVelocityChange;

	// 봇이 목표 지점에 도달했다고 간주하는 최소 거리
	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float SelfDamageInterval;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	class USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	USoundCue* ExplodeSound;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float DamageInnerRadius;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float DamageOuterRadius;

private:
	FVector NextPathPoint;
	UMaterialInstanceDynamic* MatInst;
	bool bStartedSelfDestrction = false;
	int32 PowerLevel = 0;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UPROPERTY()
	class ACoopGameMode* CoopGameMode;

	FTimerHandle TimerHandle_RefreshPath;
	FTimerHandle TimerHandle_SelfDamage;
	FTimerHandle TimerHandle_CheckPowerLevel;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	FVector GetNextPathPoint();
	void SelfDestruct();
	void ShowExplodeEffect();
	void ShowDamageEffect();
	void DamageSelf();
	void OnCheckNearByBots();
	void RefreshPath();

public:	
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const override { return LagCompensation; }
};
