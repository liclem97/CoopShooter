// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CoopGameState.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EWaveState : uint8
{	
	WaitingPlayer,		// 플레이어가 2명이 되기를 기다림.

	WaitingToStart,		// 웨이브가 시작되기를 기다림

	WaveInProgress,		// 웨이브 진행중, 봇을 소환하는 상태

	WaitingToComplete,	// 새로운 봇을 생성하지 않고, 플레이어가 남은 봇을 죽이길 기다림

	WaveComplete,		// 웨이브 클리어

	GameOver			// 플레이어가 모두 죽음, 게임 오버
};

namespace WaveStateString
{
	const FString WaitingPlayer(TEXT("Waiting Player"));
	const FString WaitingToStart(TEXT("Waiting To Start"));
	const FString WaveInProgress(TEXT("Wave In Progress"));
	const FString WaitingToComplete(TEXT("Waiting To Complete"));
	const FString WaveComplete(TEXT("Wave Complete"));
	const FString GameOver(TEXT("Game Over"));
}

UCLASS()
class COOPSHOOTER_API ACoopGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	TArray<ACoopPlayerState*> PlayerTeam;
	TArray<ACoopPlayerState*> AITeam;

	void SetWaveState(EWaveState NewState);
	EWaveState GetWaveState();

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveState, Category = "GameState")
	EWaveState WaveState = EWaveState::WaitingPlayer;

	UFUNCTION()
	void OnRep_WaveState(EWaveState OldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void WaveStateChanged(EWaveState NewState, EWaveState OldState);

private:
	float TopScore = 0.f;
};
