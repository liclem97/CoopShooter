// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopPlayerController.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/CoopComponents/CombatComponent.h"
#include "CoopShooter/CoopTypes/Announcement.h"
#include "CoopShooter/GameMode/CoopGameMode.h"
#include "CoopShooter/GameMode/LobbyGameMode.h"
#include "CoopShooter/GameState/CoopGameState.h"
#include "CoopShooter/HUD/CharacterOverlay.h"
#include "CoopShooter/HUD/CoopHUD.h"
#include "CoopShooter/HUD/GameOver.h"
#include "CoopShooter/HUD/ReturnToMainMenu.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ACoopPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	if (CoopHUD)
	{
		if (CoopHUD->CharacterOverlay == nullptr) CoopHUD->AddCharacterOverlay();
	}
}

void ACoopPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ACoopPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void ACoopPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		if (PlayerState == nullptr)
		{
			PlayerState = GetPlayerState<APlayerState>();
		}
		else
		{
			PlayerState = PlayerState;
		}
		if (PlayerState)
		{
			if ((PlayerState->GetPingInMilliseconds() / 1000) * 4 > HighPingThreshold) // GetPing() * 4
			{																		   // 핑이 압축됨, 실제 핑의 1/4
				HighPingWarnig();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		CoopHUD && CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->HighPingAnimation &&
		CoopHUD->CharacterOverlay->IsAnimationPlaying(CoopHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarnig();
		}
	}
}

void ACoopPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

// 하이핑 체크
void ACoopPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ACoopPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ACoopPlayerController::HighPingWarnig()
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->HighPingImage &&
		CoopHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		CoopHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		CoopHUD->CharacterOverlay->PlayAnimation(
			CoopHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5);
	}
}

void ACoopPlayerController::StopHighPingWarnig()
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->HighPingImage &&
		CoopHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		CoopHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (CoopHUD->CharacterOverlay->IsAnimationPlaying(CoopHUD->CharacterOverlay->HighPingAnimation))
		{
			CoopHUD->CharacterOverlay->StopAnimation(CoopHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ACoopPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(InPawn);
	if (CoopCharacter)
	{
		SetHUDHealth(CoopCharacter->GetHealth(), CoopCharacter->GetMaxHealth());
	}
}

void ACoopPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->HealthBar &&
		CoopHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		CoopHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		CoopHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ACoopPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->ShieldBar &&
		CoopHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		CoopHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		CoopHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ACoopPlayerController::SetHUDKillScore(float KillScore)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->KillAmount;
	if (bHUDValid)
	{
		FString KillScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(KillScore));
		CoopHUD->CharacterOverlay->KillAmount->SetText(FText::FromString(KillScoreText));
	}
	else
	{
		bInitializeKillScore = true;
		HUDKillScore = KillScore;
	}
}

void ACoopPlayerController::SetHUDDeathScore(int32 Death)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->DeathAmount;
	if (bHUDValid)
	{
		FString DeathText = FString::Printf(TEXT("%d"), Death);
		CoopHUD->CharacterOverlay->DeathAmount->SetText(FText::FromString(DeathText));
	}
	else
	{
		bInitializeDeath = true;
		HUDDeath = Death;
	}
}

void ACoopPlayerController::SetHUDMessage(FString Message)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->MessageText;
	if (bHUDValid)
	{
		FString NewMessageText = Message;
		CoopHUD->CharacterOverlay->MessageText->SetText(FText::FromString(NewMessageText));
	}
}

void ACoopPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		CoopHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ACoopPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		CoopHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ACoopPlayerController::SetHUDPlayTime(float PlayTime)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->PlayTimeText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(PlayTime / 60.f);
		int32 Seconds = PlayTime - Minutes * 60;

		FString PlayTimeText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CoopHUD->CharacterOverlay->PlayTimeText->SetText(FText::FromString(PlayTimeText));
	}
}

void ACoopPlayerController::SetHUDWeaponType(EWeaponType NewWeaponType)
{	
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->WeaponTypeText;
	if (bHUDValid)
	{
		FString WeaponTypeText;
		switch (NewWeaponType)
		{
		case EWeaponType::EWT_AssaultRifle:
			WeaponTypeText = WeaponTypeString::AssaultRifle;
			break;
		case EWeaponType::EWT_RocketLauncher:
			WeaponTypeText = WeaponTypeString::RocketLauncher;
			break;
		case EWeaponType::EWT_Pistol:
			WeaponTypeText = WeaponTypeString::Pistol;
			break;
		case EWeaponType::EWT_SubmachineGun:
			WeaponTypeText = WeaponTypeString::SubmachineGun;
			break;
		case EWeaponType::EWT_Shotgun:
			WeaponTypeText = WeaponTypeString::Shotgun;
			break;
		case EWeaponType::EWT_SniperRifle:
			WeaponTypeText = WeaponTypeString::SniperRifle;
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			WeaponTypeText = WeaponTypeString::GrenadeLauncher;
			break;
		default:
			WeaponTypeText = WeaponTypeString::EmptyWeapon;
			break;
		}
		CoopHUD->CharacterOverlay->WeaponTypeText->SetText(FText::FromString(WeaponTypeText));
	}
	else
	{
		bInitializeWeaponType = true;
		HUDWeaponType = NewWeaponType;
	}
}

void ACoopPlayerController::SetHUDPlayingState(FString PlayingState)
{	
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->PlayingStateText;
	if (bHUDValid)
	{
		CoopHUD->CharacterOverlay->PlayingStateText->SetText(FText::FromString(PlayingState));
	}
	else
	{
		bInitializePlayingState = true;
		HUDPlayingState = PlayingState;
	}
}

void ACoopPlayerController::SetHUDSpawnedBotNum(int32 BotNum)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->CharacterOverlay &&
		CoopHUD->CharacterOverlay->SpawnedBotNumText;
	if (bHUDValid)
	{
		FString BotNumText = FString::Printf(TEXT("%d"), BotNum);
		CoopHUD->CharacterOverlay->SpawnedBotNumText->SetText(FText::FromString(BotNumText));
	}
	else
	{
		bInitializeBotNum = true;
		HUDBotNum = BotNum;
	}
}

void ACoopPlayerController::AddGameOverWidget(float PlayTime, int32 ClearedWave, int32 Kill, int32 Death)
{	
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	if (CoopHUD)
	{
		if (CoopHUD->GameOver == nullptr) CoopHUD->AddGameOver();
		bool bHUDVaild = CoopHUD->GameOver &&
			CoopHUD->GameOver->PlayTimeText &&
			CoopHUD->GameOver->ClearWaveText &&
			CoopHUD->GameOver->KillText &&
			CoopHUD->GameOver->DeathText;
		if (bHUDVaild)
		{	
			int32 Minutes = FMath::FloorToInt(PlayTime / 60.f);
			int32 Seconds = PlayTime - Minutes * 60;

			FString PlayTimeText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			FString ClearedWaveText = FString::Printf(TEXT("%d"), ClearedWave);
			FString KillText = FString::Printf(TEXT("%d"), Kill);
			FString DeathText = FString::Printf(TEXT("%d"), Death);

			CoopHUD->GameOver->PlayTimeText->SetText(FText::FromString(PlayTimeText));
			CoopHUD->GameOver->ClearWaveText->SetText(FText::FromString(ClearedWaveText));
			CoopHUD->GameOver->KillText->SetText(FText::FromString(KillText));
			CoopHUD->GameOver->DeathText->SetText(FText::FromString(DeathText));
		}		
	}
}

void ACoopPlayerController::RemoveCharacterOverlay()
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	if (CoopHUD && CoopHUD->CharacterOverlay)
	{	
		UE_LOG(LogTemp, Display, TEXT("PlayerController RemoveCharacterOverlay"));
		CoopHUD->RemoveCharacterOverlay();
	}
}

void ACoopPlayerController::SetGameoverHUDAnnouncement(int32 CountdownTime)
{
	CoopHUD = CoopHUD == nullptr ? Cast<ACoopHUD>(GetHUD()) : CoopHUD;
	bool bHUDValid = CoopHUD &&
		CoopHUD->GameOver &&
		CoopHUD->GameOver->AnnouncementText;
	if (bHUDValid)
	{
		FString AnnouncementText = FString::Printf(TEXT("Go to the main menu after %d seconds"), CountdownTime);
		CoopHUD->GameOver->AnnouncementText->SetText(FText::FromString(AnnouncementText));
	}
	else
	{
		bInitializeCountdown = true;
		HUDCountdownTime = CountdownTime;
	}
}

void ACoopPlayerController::SetHUDTime()
{
	// 게임 플레이 시작 시간을 가져옵니다.
	float PlayingTime = GetServerTime() + LevelStartingTime;
	
	SetHUDPlayTime(PlayingTime);
}

void ACoopPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (CoopHUD && CoopHUD->CharacterOverlay)
		{
			CharacterOverlay = CoopHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth)		SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)		SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeKillScore)	SetHUDKillScore(HUDKillScore);
				if (bInitializeDeath)		SetHUDDeathScore(HUDDeath);
				if (bInitializeCarriedAmmo)	SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo)	SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (bInitializeWeaponType)	SetHUDWeaponType(HUDWeaponType);
				if (bInitializePlayingState)SetHUDPlayingState(HUDPlayingState);
				if (bInitializeBotNum)		SetHUDSpawnedBotNum(HUDBotNum);
			}
		}
	}
	if (GameOver == nullptr)
	{
		if (CoopHUD && CoopHUD->GameOver)
		{
			GameOver = CoopHUD->GameOver;
			if (GameOver)
			{
				if (bInitializeCountdown)	SetGameoverHUDAnnouncement(HUDCountdownTime);
			}
		}
	}
}

void ACoopPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ACoopPlayerController::ShowReturnToMainMenu);
}

void ACoopPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ACoopPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ACoopPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ACoopPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}