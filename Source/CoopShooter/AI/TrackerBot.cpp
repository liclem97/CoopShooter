// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackerBot.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/CoopComponents/LagCompensationComponent.h"
#include "CoopShooter/CoopShooter.h"
#include "CoopShooter/GameMode/CoopGameMode.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBotDrawing(
	TEXT("COOP.DebugTrackerBot"),
	DebugTrackerBotDrawing,
	TEXT("Draw Debug Lines for TrackerBot,"),
	ECVF_Cheat);

ATrackerBot::ATrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_SkeletalMesh);
	MeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	RootComponent = MeshComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitCollisionBoxes.Add(FName("HitBox"), HitBox);
	HitBox->SetCollisionObjectType(ECC_HitBox);
	HitBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBox->SetupAttachment(RootComponent);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovement"));

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));

	bUseVelocityChange = false;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionDamage = 60; 
	SelfDamageInterval = 0.25f;
	
	TeamNum = TeamNum::AITeam;

	DamageInnerRadius = 200.f;
	DamageOuterRadius = 500.f;
}

void ATrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATrackerBot, Health);
}

void ATrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	CoopGameMode = CoopGameMode == nullptr ? Cast<ACoopGameMode>(GetWorld()->GetAuthGameMode()) : CoopGameMode;
	if (CoopGameMode)
	{
		CoopGameMode->AddSpawnedBotNum();
	}
	if (HasAuthority())
	{	
		OnTakeAnyDamage.AddDynamic(this, &ATrackerBot::ReceiveDamage);
		NextPathPoint = GetNextPathPoint();

		// 1초마다 확인, 주위에 다른 봇들이 더 있으면 대미지가 증가함.
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ATrackerBot::OnCheckNearByBots, 1.0f, true);
	}	
}

void ATrackerBot::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{	
	// Explode on hitpoint == 0
	CoopGameMode = CoopGameMode == nullptr ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : CoopGameMode;
	if (bElimmed || CoopGameMode == nullptr) return;
	if (IsFriendly(DamageCauser->GetOwner(), DamagedActor)) return;
	else
	{
		float DamageToHealth = Damage;
		Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
		ShowDamageEffect();

		if (Health == 0.0f)
		{	
			ACoopPlayerController* AttackerController = Cast<ACoopPlayerController>(InstigatorController);
			CoopGameMode->PawnEliminated(this, Controller, AttackerController);
			SelfDestruct();
		}
	}
}

void ATrackerBot::OnRep_Health(float LastHealth)
{	
	if (Health < LastHealth)
	{	
		ShowDamageEffect();
	}
}

// 플레이어에게 가는 경로의 다음 포인트를 알려줌.
FVector ATrackerBot::GetNextPathPoint()
{	
	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX; // 가장 큰 값으로 초기화

	for (TActorIterator<ACoopCharacter> It(GetWorld()); It; ++It)
	{
		ACoopCharacter* CoopCharacter = *It;
		if (CoopCharacter == nullptr ||							// null 포인터
			CoopCharacter->IsFriendly(this, CoopCharacter) ||	// 같은 팀
			CoopCharacter->IsElimmed()) continue;				// 이미 죽었으면 건너뜀.

		if (CoopCharacter &&									// 유효함
			!CoopCharacter->IsFriendly(this, CoopCharacter) &&	// 다른 팀
			!CoopCharacter->IsElimmed())						// 죽지 않았으면 BestTarget 설정
		{
			float Distance = (CoopCharacter->GetActorLocation() - GetActorLocation()).Size();
			if (Distance < NearestTargetDistance)
			{
				BestTarget = CoopCharacter;
				NearestTargetDistance = Distance;
			}
		}
	}

	if (BestTarget)
	{
		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ATrackerBot::RefreshPath, 5.0f, false);

		if (NavPath->PathPoints.Num() > 1)
		{
			return NavPath->PathPoints[1];
		}
	}

	// 길 찾기 실패
	return GetActorLocation();
}

// 자폭 공격.
void ATrackerBot::SelfDestruct()
{	
	if (bElimmed) return;
	bElimmed = true;

	if (HasAuthority())
	{	
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this,
			ActualDamage,
			10.f,
			GetActorLocation(),
			DamageInnerRadius,
			DamageOuterRadius,
			1.f,
			UDamageType::StaticClass(),
			IgnoredActors,
			this,
			GetInstigatorController()
		);

		if (DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), DamageOuterRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);
		}

		Destroy();
	}
}

void ATrackerBot::ShowExplodeEffect()
{	
	if (ExplodeSound && ExplosionEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	}
}

void ATrackerBot::ShowDamageEffect()
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}
}

void ATrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ATrackerBot::OnCheckNearByBots()
{
	const float Radius = 600;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;

	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	if (DebugTrackerBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);
	}

	int32 NrOfBots = 0;

	for (FOverlapResult Result : Overlaps)
	{
		ATrackerBot* Bot = Cast<ATrackerBot>(Result.GetActor());
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatInst)
	{
		// 주위에 다른 봇이 있을 경우 머티리얼 인스턴스의 PowerLevelAlpha 값 증가.
		float Alpha = PowerLevel / (float)MaxPowerLevel;

		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}

	if (DebugTrackerBotDrawing)
	{
		DrawDebugString(GetWorld(), FVector(0, 0, 0), FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);
	}
}

void ATrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bElimmed)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();
			if (DebugTrackerBotDrawing)
			{
				DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached");
			}
		}
		else
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			if (DebugTrackerBotDrawing)
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
			}
		}
		if (DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 4.0f, 1.0f);
		}
	}
}

void ATrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestrction && !bElimmed)
	{
		ACoopCharacterBase* CoopCharacter = Cast<ACoopCharacterBase>(OtherActor);
		if (CoopCharacter && CoopCharacter->GetTeamNum() != TeamNum::AITeam)
		{
			// 플레이어와 오버랩 된 상태.
			if (HasAuthority())
			{
				// 자폭 시퀀스 시작.
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ATrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}

			bStartedSelfDestrction = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}
}

void ATrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

void ATrackerBot::Destroyed()
{
	Super::Destroyed();
	ShowExplodeEffect();
	CoopGameMode = CoopGameMode == nullptr ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : CoopGameMode;
	if (CoopGameMode)
	{
		CoopGameMode->SubSpawnedBotNum();
	}
}