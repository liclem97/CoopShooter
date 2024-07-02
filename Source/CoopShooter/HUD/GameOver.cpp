// Fill out your copyright notice in the Description page of Project Settings.


#include "GameOver.h"
#include "Components/Button.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/Character/CoopSpectatorPawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerSessionsSubsystem.h"

void UGameOver::NativePreConstruct()
{	
	Super::NativePreConstruct();

	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	if (GoToMainMenuButton && !GoToMainMenuButton->OnClicked.IsBound())
	{
		GoToMainMenuButton->OnClicked.AddDynamic(this, &UGameOver::GoToMainMenuButtonClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UGameOver::OnDestroySession);
		}
	}
}

bool UGameOver::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UGameOver::OnDestroySession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		GoToMainMenuButton->SetIsEnabled(true);
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode) // server
		{
			GameMode->ReturnToMainMenuHost();
		}
		else // client
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UGameOver::GoToMainMenuButtonClicked()
{
	GoToMainMenuButton->SetIsEnabled(false);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			ACoopSpectatorPawn* CoopPawn = Cast<ACoopSpectatorPawn>(FirstPlayerController->GetPawn());
			ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(FirstPlayerController->GetPawn());
			if (CoopPawn)
			{	
				// 플레이어가 모두 죽어서 관전자 폰 인경우 실행한다.
				CoopPawn->ServerLeaveGame();
				CoopPawn->OnLeftGame.AddDynamic(this, &UGameOver::OnPlayerLeftGame);
			}
			else if (CoopCharacter)
			{
				// 게임이 클리어 되어 살아있는 캐릭터인 경우 실행한다.
				CoopCharacter->ServerLeaveGame();
				CoopCharacter->OnLeftGame.AddDynamic(this, &UGameOver::OnPlayerLeftGame);
			}
			else
			{
				GoToMainMenuButton->SetIsEnabled(true);
			}
		}
	}
}

void UGameOver::OnPlayerLeftGame()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}