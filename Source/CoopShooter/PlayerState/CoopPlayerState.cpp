// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopPlayerState.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/GameState/CoopGameState.h"
#include "CoopShooter/HUD/CoopHUD.h"
#include "CoopShooter/HUD/GameOver.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

void ACoopPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACoopPlayerState, Death);
	DOREPLIFETIME(ACoopPlayerState, Message);
	DOREPLIFETIME(ACoopPlayerState, SpawnedBotNum);
	DOREPLIFETIME(ACoopPlayerState, PlayingState);
	DOREPLIFETIME(ACoopPlayerState, bShowGameOverWidget);
	DOREPLIFETIME(ACoopPlayerState, ClearedWave);
	DOREPLIFETIME(ACoopPlayerState, PlayTime);
	DOREPLIFETIME(ACoopPlayerState, CountdownTime);
}

void ACoopPlayerState::AddToKill(float KillAmount)
{	
	// 캐릭터 오버레이의 킬 수를 업데이트한다.
	SetScore(GetScore() + KillAmount);
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDKillScore(GetScore());
		}
	}
}

void ACoopPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDKillScore(GetScore());
		}
	}
}

void ACoopPlayerState::AddToDeath(int32 DeathAmount)
{	
	// 캐릭터 오버레이의 데스 수를 업데이트 한다.
	Death += DeathAmount;
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDeathScore(Death);
		}
	}
}

void ACoopPlayerState::OnRep_Death()
{
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDeathScore(Death);
		}
	}
}

void ACoopPlayerState::SetServerMessage(FString NewMessage)
{	
	// 캐릭터 오버레이의 서버 메시지를 설정한다.
	Message = NewMessage;
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDMessage(Message);
		}
	}
}

void ACoopPlayerState::OnRep_Message()
{
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDMessage(Message);
		}
	}
}

void ACoopPlayerState::UpdateSpawnedBotNum(int32 NewBotNum)
{	
	// 캐릭터 오버레이의 봇 숫자를 업데이트한다.
	SpawnedBotNum = NewBotNum;
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDSpawnedBotNum(SpawnedBotNum);
		}
	}
}

void ACoopPlayerState::OnRep_SpawnedBotNum()
{
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDSpawnedBotNum(SpawnedBotNum);
		}
	}
}

void ACoopPlayerState::UpdatePlayingState(FString PlayingStateText)
{	
	// 캐릭터 오버레이의 플레잉스테이트 텍스트를 업데이트 한다.
	PlayingState = PlayingStateText;
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDPlayingState(PlayingState);
		}
	}
}

void ACoopPlayerState::AddGameOverWidget(bool bShowWidget, float NewPlayTime, int32 NewClearedWave, int32 NewCountdownTime)
{	
	bShowGameOverWidget = bShowWidget;
	PlayTime = NewPlayTime;
	ClearedWave = NewClearedWave;
	CountdownTime = NewCountdownTime;
	if (bShowGameOverWidget)
	{
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{	
				Controller->RemoveCharacterOverlay();
				Controller->AddGameOverWidget(PlayTime, ClearedWave, GetScore(), Death);
				GetWorldTimerManager().SetTimer(TimerHandle_Countdown, this, &ACoopPlayerState::UpdateAnnouncementText, 1.f, true);
			}
		}
	}
}

void ACoopPlayerState::UpdateAnnouncementText()
{	
	CountdownTime--;

	if (CountdownTime > 0)
	{	
		// 게임오버 위젯의 안내 메시지를 업데이트한다.
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{
				Controller->SetGameoverHUDAnnouncement(CountdownTime);
			}
		}
	}
	else
	{
		// 카운트다운 타임이 끝나면 플레이어를 메인메뉴로 내보낸다.
		GetWorldTimerManager().ClearTimer(TimerHandle_Countdown);
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{
				ACoopHUD* CoopHUD = Controller->GetCoopHUD();
				if (CoopHUD && CoopHUD->GetGameOverWidget())
				{
					CoopHUD->GetGameOverWidget()->GoToMainMenuButtonClicked();
				}
			}
		}
	}
}

void ACoopPlayerState::OnRep_PlayingState()
{
	Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDPlayingState(PlayingState);
		}
	}
}

void ACoopPlayerState::OnRep_bShowGameOverWidget()
{	
	if (bShowGameOverWidget)
	{
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{	
				Controller->RemoveCharacterOverlay();
				Controller->AddGameOverWidget(PlayTime, ClearedWave, GetScore(), Death);
			}
		}
	}
}

void ACoopPlayerState::OnRep_ClearedWave()
{
}

void ACoopPlayerState::OnRep_PlayTime()
{
}

void ACoopPlayerState::OnRep_CountdownTime()
{
	if (CountdownTime > 0)
	{
		// 게임오버 위젯의 안내 메시지를 업데이트한다.
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{
				Controller->SetGameoverHUDAnnouncement(CountdownTime);
			}
		}
	}
	else
	{
		// 카운트다운 타임이 끝나면 플레이어를 메인메뉴로 내보낸다.
		GetWorldTimerManager().ClearTimer(TimerHandle_Countdown);
		Character = Character == nullptr ? Cast<ACoopCharacter>(GetPawn()) : Character;
		if (Character)
		{
			Controller = Controller == nullptr ? Cast<ACoopPlayerController>(Character->Controller) : Controller;
			if (Controller)
			{
				ACoopHUD* CoopHUD = Controller->GetCoopHUD();
				if (CoopHUD && CoopHUD->GetGameOverWidget())
				{
					CoopHUD->GetGameOverWidget()->GoToMainMenuButtonClicked();
				}
			}
		}
	}
}
