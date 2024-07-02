// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoopShooter/CoopTypes/Team.h"
#include "CoopShooter/Interfaces/InteractWithCrosshairsInterface.h"
#include "GameFramework/Character.h"
#include "CoopCharacterBase.generated.h"

UCLASS()
class COOPSHOOTER_API ACoopCharacterBase : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ACoopCharacterBase();

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	UFUNCTION(BlueprintPure, BlueprintCallable)
	bool IsFriendly(AActor* Attacker, AActor* Victim);
protected:
	/**
	* Components
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	uint8 TeamNum = TeamNum::NoTeam;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bElimmed = false;

public:
	virtual class AWeapon* GetEquippedWeapon();
	FORCEINLINE virtual UCombatComponent* GetCombat() const { return nullptr; }
	FORCEINLINE virtual UBuffComponent* GetBuff() const { return nullptr; }
	FORCEINLINE virtual ULagCompensationComponent* GetLagCompensation() const { return nullptr; }	
	FORCEINLINE virtual void SetOverlappingWeapon(AWeapon* Weapon) {}
	FORCEINLINE uint8 GetTeamNum() const { return TeamNum; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsElimmed() const { return bElimmed; }	
};
