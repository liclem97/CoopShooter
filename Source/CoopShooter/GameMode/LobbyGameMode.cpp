// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerSessionsSubsystem.h"
#include "TimerManager.h"

ALobbyGameMode::ALobbyGameMode()
{   
    CountdownTime = 5; // 초기 카운트다운 시간을 설정합니다.
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    HandlePostLogin(NewPlayer);
    StartPlayerCountCheck();
}

void ALobbyGameMode::PlayerLeftGame(ACoopPlayerState* PlayerLeaving)
{
    if (PlayerLeaving == nullptr) return;
    ACoopCharacter* CharacterLeaving = Cast<ACoopCharacter>(PlayerLeaving->GetPawn());
    if (CharacterLeaving)
    {
        CharacterLeaving->Elim(true);
    }
}

void ALobbyGameMode::HandlePostLogin(APlayerController* NewPlayer)
{
    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
        check(Subsystem);

        if (NumberOfPlayers == Subsystem->DesiredNumPublicConnections)
        {
            StartMatchCountdown();
        }
    }
}

void ALobbyGameMode::CheckPlayerNum()
{
    int32 PlayerCount = GetNumPlayers();
    if (PlayerCount < 2)
    {
        // 플레이어 수가 2명 미만일 경우 메시지를 보냅니다.
        NotifyWaitingForPlayers();
    }
    else
    {
        StopPlayerCountCheck(); // 플레이어가 2명 이상일 때 타이머를 멈춥니다.
    }
}

void ALobbyGameMode::NotifyWaitingForPlayers()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
        if (CoopPC)
        {
            ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
            if (CoopPS)
            {
                CoopPS->SetServerMessage("Waiting for other players...");
            }
        }
    }
}

void ALobbyGameMode::StartPlayerCountCheck()
{
    GetWorld()->GetTimerManager().SetTimer(PlayerCountCheckTimerHandle, this, &ALobbyGameMode::CheckPlayerNum, 1.0f, true);
}

void ALobbyGameMode::StopPlayerCountCheck()
{
    GetWorld()->GetTimerManager().ClearTimer(PlayerCountCheckTimerHandle);
}

void ALobbyGameMode::StartMatchCountdown()
{
    GetWorld()->GetTimerManager().SetTimer(MatchCountdownTimerHandle, this, &ALobbyGameMode::NotifyCountdown, 1.0f, true);
}

void ALobbyGameMode::NotifyCountdown()
{
    if (CountdownTime > 0)
    {
        FString CountdownMessage = FString::Printf(TEXT("Match starting in %d seconds..."), CountdownTime);
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
            if (CoopPC)
            {
                ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
                if (CoopPS)
                {
                    CoopPS->SetServerMessage(CountdownMessage);
                }
            }
        }
        CountdownTime--;
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(MatchCountdownTimerHandle);
        UWorld* World = GetWorld();
        if (World)
        {
            bUseSeamlessTravel = true;
            World->ServerTravel(FString("/Game/Maps/PlayMap?listen"));
        }
    }
}