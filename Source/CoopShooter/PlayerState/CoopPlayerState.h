// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CoopShooter/CoopTypes/Team.h"
#include "CoopPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class COOPSHOOTER_API ACoopPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	/**
	*	Replication notifies.
	*/
	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Death();

	UFUNCTION()
	virtual void OnRep_Message();

	UFUNCTION()
	void OnRep_SpawnedBotNum();

	UFUNCTION()
	void OnRep_PlayingState();

	UFUNCTION()
	void OnRep_bShowGameOverWidget();

	UFUNCTION()
	void OnRep_ClearedWave();

	UFUNCTION()
	void OnRep_PlayTime();

	UFUNCTION()
	void OnRep_CountdownTime();

	void AddToKill(float KillAmount);
	void AddToDeath(int32 DeathAmount);
	void SetServerMessage(FString NewMessage);
	void UpdateSpawnedBotNum(int32 NewBotNum);
	void UpdatePlayingState(FString PlayingStateText);

	void AddGameOverWidget(bool bShowWidget, float NewPlayTime, int32 NewClearedWave, int32 NewCountdownTime);
	void UpdateAnnouncementText();

private:
	UPROPERTY()
	class ACoopCharacter* Character;
	UPROPERTY()
	class ACoopPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Death)
	int32 Death;

	UPROPERTY(ReplicatedUsing = OnRep_Message)
	FString Message;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnedBotNum)
	int32 SpawnedBotNum;

	UPROPERTY(ReplicatedUsing = OnRep_PlayingState)
	FString PlayingState;

	UPROPERTY(ReplicatedUsing = OnRep_bShowGameOverWidget)
	bool bShowGameOverWidget = false;

	UPROPERTY(ReplicatedUsing = OnRep_ClearedWave)
	int32 ClearedWave;

	UPROPERTY(ReplicatedUsing = OnRep_PlayTime)
	float PlayTime;

	UPROPERTY(ReplicatedUsing = OnRep_CountdownTime)
	int32 CountdownTime;

	FTimerHandle TimerHandle_Countdown;

public:

};

