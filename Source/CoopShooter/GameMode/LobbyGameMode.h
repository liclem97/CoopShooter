// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COOPSHOOTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
    void PlayerLeftGame(class ACoopPlayerState* PlayerLeaving);

private:
    ALobbyGameMode();

    void CheckPlayerNum();
    void NotifyWaitingForPlayers();
    void StartPlayerCountCheck();
    void StopPlayerCountCheck();
    void NotifyCountdown();
    void HandlePostLogin(APlayerController* NewPlayer);

    UFUNCTION()
    void StartMatchCountdown();

    FTimerHandle PlayerCountCheckTimerHandle;
    FTimerHandle MatchCountdownTimerHandle;

    int32 CountdownTime;
};
