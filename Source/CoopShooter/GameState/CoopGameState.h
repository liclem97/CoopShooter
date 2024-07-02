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
	WaitingPlayer,		// �÷��̾ 2���� �Ǳ⸦ ��ٸ�.

	WaitingToStart,		// ���̺갡 ���۵Ǳ⸦ ��ٸ�

	WaveInProgress,		// ���̺� ������, ���� ��ȯ�ϴ� ����

	WaitingToComplete,	// ���ο� ���� �������� �ʰ�, �÷��̾ ���� ���� ���̱� ��ٸ�

	WaveComplete,		// ���̺� Ŭ����

	GameOver			// �÷��̾ ��� ����, ���� ����
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
