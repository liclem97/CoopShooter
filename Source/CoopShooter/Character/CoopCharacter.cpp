// Fill out your copyright notice in the Description page of Project Settings.

#include "CoopCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "CoopAnimInstance.h"
#include "CoopShooter/CoopComponents/BuffComponent.h"
#include "CoopShooter/CoopComponents/CombatComponent.h"
#include "CoopShooter/CoopComponents/LagCompensationComponent.h"
#include "CoopShooter/CoopShooter.h"
#include "CoopShooter/GameMode/CoopGameMode.h"
#include "CoopShooter/GameMode/LobbyGameMode.h"
#include "CoopShooter/GameState/CoopGameState.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"
#include "CoopShooter/Weapon/Weapon.h"
#include "CoopShooter/Weapon/WeaponTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"

ACoopCharacter::ACoopCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	// 서버 리와인드를 위한 히트 박스 설정
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	// 히트 박스의 충돌 설정
	for (auto Box : HitCollisionBoxes)
	{	
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ACoopCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACoopCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ACoopCharacter, Health);
	DOREPLIFETIME(ACoopCharacter, Shield);
	DOREPLIFETIME(ACoopCharacter, bDisableGameplay);
}

void ACoopCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ACoopCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	MulticastElim(bPlayerLeftGame);
}

void ACoopCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	InitializeCoopPlayerController();

	bElimmed = true;
	PlayElimMontage();

	// 녹는 효과 시작.
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		GetMesh()->SetMaterial(1, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// 캐릭터의 움직임을 비활성화합니다.
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	// 충돌 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 제거 효과 스폰
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	// 스나이퍼 스코프 비활성화
	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ACoopCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ACoopCharacter::ElimTimerFinished()
{
	InitializeCoopGameMode();
	InitializeCoopPlayerController();
	if (CoopGameMode && CoopPlayerController && !bLeftGame)
	{	
		SpawnSpectatorPawn();
		Destroy(); 
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

// 서버에서 플레이어가 게임을 떠날 때 호출됩니다.
void ACoopCharacter::ServerLeaveGame_Implementation()
{
	InitializeCoopGameMode();
	LobbyGameMode = LobbyGameMode == nullptr ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : LobbyGameMode;
	CoopPlayerState = CoopPlayerState == nullptr ? GetPlayerState<ACoopPlayerState>() : CoopPlayerState;
	if (CoopGameMode && CoopPlayerState)
	{
		CoopGameMode->PlayerLeftGame(CoopPlayerState);
	}
	else if (LobbyGameMode && CoopPlayerState)
	{	
		//로비에서 맵을 떠날때 호출
		LobbyGameMode->PlayerLeftGame(CoopPlayerState);
	}
}

void ACoopCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ACoopCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

void ACoopCharacter::OnPlayerStateInitialized()
{	
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		CoopPlayerState->AddToKill(0.f);
		CoopPlayerState->AddToDeath(0);
		CoopPlayerState->SetServerMessage(TEXT(""));
	}
	SetTeamColor(TeamNum);
}

void ACoopCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	InitializeCoopGameMode();
	
	// 게임이 진행 중이 아닌지 확인
	bool bMatchNotInProgress = CoopGameMode && CoopGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ACoopCharacter::SetTeamColor(uint8 NewTeamNum)
{
	if (GetMesh() == nullptr || OriginalMaterial == nullptr) return;
	switch (NewTeamNum)
	{
	case TeamNum::NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		GetMesh()->SetMaterial(1, OriginalMaterial);
		DissolveMaterialInstance = PlayerDissolveMatInst;
		break;
	case TeamNum::PlayerTeam:
		GetMesh()->SetMaterial(0, PlayerTeamMaterial);
		GetMesh()->SetMaterial(1, PlayerTeamMaterial);
		DissolveMaterialInstance = PlayerDissolveMatInst;
		break;
	case TeamNum::AITeam:
		GetMesh()->SetMaterial(0, AITeamMaterial);
		GetMesh()->SetMaterial(1, AITeamMaterial);
		DissolveMaterialInstance = AIDissolveMatInst;
		break;
	}
}

void ACoopCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		UpdateHUDAmmo();
		UpdateHUDHealth();
		UpdateHUDShield();
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ACoopCharacter::ReceiveDamage);
	}
}

void ACoopCharacter::InitializeCoopPlayerController()
{
	CoopPlayerController = CoopPlayerController == nullptr ? Cast<ACoopPlayerController>(Controller) : CoopPlayerController;
}

void ACoopCharacter::InitializeCoopGameMode()
{
	CoopGameMode = CoopGameMode == nullptr ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : CoopGameMode;
}

void ACoopCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	if (CoopPlayerController)
	{
		HideCharacterIfCameraClose(CoopPlayerController);
	}
	PollInit();
}

// 캐릭터가 제자리에서 회전하도록 처리합니다.
void ACoopCharacter::RotateInPlace(float DeltaTime)
{
	// 전투 컴포넌트와 장착된 무기가 유효한 경우, 이동 방향에 따라 회전하지 않도록 설정
	if (Combat && Combat->EquippedWeapon)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}

	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// 로컬 플레이어가 컨트롤하는 경우
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{	
		// 마지막 이동 복제 이후 경과 시간 업데이트
		TimeSinceLastMovementReplication += DeltaTime;

		// 일정 시간이 경과한 경우, 이동 복제 처리
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ACoopCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACoopCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACoopCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACoopCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ACoopCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ACoopCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ACoopCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ACoopCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ACoopCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ACoopCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ACoopCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ACoopCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ACoopCharacter::ReloadButtonPressed);
}

void ACoopCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ACoopPlayerController>(Controller);
		}
	}
}

void ACoopCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ACoopCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ACoopCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ACoopCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void ACoopCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage && Combat->CombatState != ECombatState::ECS_Reloading)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ACoopCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	InitializeCoopGameMode();
	if (bElimmed || CoopGameMode == nullptr) return;
	if (IsFriendly(DamageCauser->GetOwner(), DamagedActor)) return;
	
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		if (CoopGameMode)
		{
			InitializeCoopPlayerController();
			ACoopPlayerController* AttackerController = Cast<ACoopPlayerController>(InstigatorController);
			CoopGameMode->PawnEliminated(this, CoopPlayerController, AttackerController);
		}
	}
}

void ACoopCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ACoopCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ACoopCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ACoopCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ACoopCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat && bCanE)
	{	
		// 무기 장착 버튼을 누를 수 없도록 설정
		bCanE = false;

		// Combat 상태가 Unoccupied인 경우 서버에 무기 장착 요청
		if (Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			ServerEquipButtonPressed();
		}

		bool bSwap = Combat->ShouldSwapWeapons() &&
			!HasAuthority() &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;

		if (bSwap)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}

		// 무기 장착 버튼의 쿨타임 리셋
		ResetECooldown();
	}
}

void ACoopCharacter::ResetECooldown()
{
	if (bCanE) return;
	GetWorldTimerManager().SetTimer(TimerHandler_ECoolDown, this, &ACoopCharacter::CanPressE, CooldownTime, false);
}

void ACoopCharacter::CanPressE()
{
	bCanE = true;
}

void ACoopCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ACoopCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ACoopCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void ACoopCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ACoopCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

float ACoopCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ACoopCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// 정지 상태이면서 공중에 있지 않은 경우
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ACoopCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// 피치 값을 [270, 360) 범위에서 [-90, 0) 범위로 매핑
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ACoopCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ACoopCharacter::Jump()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ACoopCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (bElimmed) return;
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ACoopCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (bElimmed) return;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ACoopCharacter::AIStartFire()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->AIStartFire();
	}
}

void ACoopCharacter::AIStopFire()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->AIStopFire();
	}
}

void ACoopCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ACoopCharacter::HideCharacterIfCameraClose(AController* PlayerController)
{
	ACoopPlayerController* CoopController = Cast<ACoopPlayerController>(PlayerController);
	if (!IsLocallyControlled() || !CoopController)	return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraTheshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ACoopCharacter::OnRep_Health(float LastHealth)
{	
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		UpdateHUDHealth();
	}	
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ACoopCharacter::OnRep_Shield(float LastShield)
{
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		UpdateHUDShield();
	}
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ACoopCharacter::UpdateHUDHealth()
{
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		CoopPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ACoopCharacter::UpdateHUDShield()
{
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		CoopPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ACoopCharacter::UpdateHUDAmmo()
{
	InitializeCoopPlayerController();
	if (CoopPlayerController && Combat && Combat->EquippedWeapon)
	{
		CoopPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		CoopPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ACoopCharacter::SpawnDefaultWeapon()
{
	InitializeCoopGameMode();
	UWorld* World = GetWorld();
	if (CoopGameMode && World && !bElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		if (StartingWeapon)
		{
			StartingWeapon->bDestroyWeapon = true;
			if (Combat)
			{
				Combat->EquipWeapon(StartingWeapon);
			}
		}
	}
}

void ACoopCharacter::PollInit()
{
	if (CoopPlayerState == nullptr)
	{
		CoopPlayerState = GetPlayerState<ACoopPlayerState>();
		if (CoopPlayerState)
		{
			OnPlayerStateInitialized();
		}
	}
}

void ACoopCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ACoopCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ACoopCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ACoopCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ACoopCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ACoopCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ACoopCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ACoopCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ACoopCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState ACoopCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

bool ACoopCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}

FVector ACoopCharacter::GetPawnViewLocation() const
{	
	if (FollowCamera)
	{
		return FollowCamera->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

void ACoopCharacter::SpawnSpectatorPawn()
{
	if (SpectatorPawnClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpectatorPawnClass이 설정 되지 않음."));
		return;
	}

	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* SpawnedSpectatorPawn = GetWorld()->SpawnActor<APawn>(SpectatorPawnClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (SpawnedSpectatorPawn == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpectatorPawn을 스폰하는데 실패함."));
		return;
	}
	
	InitializeCoopPlayerController();
	if (CoopPlayerController)
	{
		CoopPlayerController->UnPossess();
		CoopPlayerController->Possess(SpawnedSpectatorPawn);
	}	
}