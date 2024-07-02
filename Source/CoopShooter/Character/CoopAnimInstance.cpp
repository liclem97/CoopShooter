// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopAnimInstance.h"
#include "CoopCharacter.h"
#include "CoopShooter/CoopTypes/CombatState.h"
#include "CoopShooter/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCoopAnimInstance::NativeInitializeAnimation()
{	
	Super::NativeInitializeAnimation();

	CoopCharacter = Cast<ACoopCharacter>(TryGetPawnOwner());
}

// 애니메이션 인스턴스를 업데이트합니다.
void UCoopAnimInstance::NativeUpdateAnimation(float DeltaTime)
{	
	Super::NativeUpdateAnimation(DeltaTime);

	if (CoopCharacter == nullptr)
	{
		CoopCharacter = Cast<ACoopCharacter>(TryGetPawnOwner());
	}
	if (CoopCharacter == nullptr) return;

	FVector Velocity = CoopCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();	

	bIsInAir = CoopCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = CoopCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = CoopCharacter->IsWeaponEquipped();
	EquippedWeapon = CoopCharacter->GetEquippedWeapon();

	bIsCrouched = CoopCharacter->bIsCrouched;

	bAiming = CoopCharacter->IsAiming();

	TurningInPlace = CoopCharacter->GetTurningInPlace();

	bRotateRootBone = CoopCharacter->ShouldRotateRootBone();

	bElimmed = CoopCharacter->IsElimmed();	
	
	// 횡 이동을 위한 Yaw 오프셋 계산
	FRotator AimRotation = CoopCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(CoopCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// 기울기(Lean) 계산
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = CoopCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	// 조준 오프셋 계산
	AO_Yaw = CoopCharacter->GetAO_Yaw();
	AO_Pitch = CoopCharacter->GetAO_Pitch();

	// FABRIK 설정
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && CoopCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		CoopCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (CoopCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - CoopCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	bUseFABRIK = CoopCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bool bFABRIKOverride = CoopCharacter->IsLocallyControlled() &&
		CoopCharacter->bFinishedSwapping;
	if (bFABRIKOverride)
	{
		bUseFABRIK = !CoopCharacter->IsLocallyReloading();
	}
	bUseAimOffsets = CoopCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !CoopCharacter->GetDisableGameplay();
	bTransformRightHand = CoopCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !CoopCharacter->GetDisableGameplay();
}
