// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CoopGameMode.generated.h"

namespace MatchState
{
	extern COOPSHOOTER_API const FName Cooldown; // 경기의 끝. 승자를 표시하고 쿨다운 타이머를 시작 함.
}

enum class EWaveState : uint8;

USTRUCT(BlueprintType)
struct FWaveData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TrackerBotSpawnNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SGAISpawnNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SRAISpawnNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GLAISpawnNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RLAISpawnNum;
};
/**
 * 
 */
UCLASS()
class COOPSHOOTER_API ACoopGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ACoopGameMode();
	virtual void Tick(float DeltaTime) override;
	void PawnEliminated(class ACoopCharacterBase* ElimmedPawn, class AController* VictimController, AController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(class ACoopPlayerState* PlayerLeaving);
	void UpdateSpawnedBotNum(int32 NewSpawnedBotNum);
	void BotNumChanged(int32 NewBotNum);
	FString GetWaveStateText(EWaveState NewWaveState, int32 NewWaveCount);
	void UpdatePlayingState(EWaveState NewWaveState, int32 NewWaveCount);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	bool bTeamsMatch = false;

	FTimerHandle TimerHandle_BotSpawner;

	// 웨이브에서 스폰해야 할 봇의 수
	int32 NumOfBotsToSpawn;

	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;
	FTimerHandle TimerHandle_NextWaveStart;

	void PlayerRespawnAndDestroySpectator();

	void SetSpawnEnemyNum();

	void StartPlayTime();
	void EndPlayTime();
	float GetPlayTime() const;


protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();

	void WaitingToStart();

	void StartWave();

	UFUNCTION(BlueprintCallable)
	void EndWave();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void CheckBotNum();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void WaitingOtherPlayer();

	int32 SpawnedBotNum;
	int32 CurrentBotNum;

	UPROPERTY(BlueprintReadWrite)
	int32 TrackerBotSpawnNum = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 SGAISpawnNum = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 SRAISpawnNum = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 GLAISpawnNum = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 RLAISpawnNum = 0;

	UPROPERTY(EditAnywhere)
	float CountdownTime;

	UPROPERTY(EditAnywhere)
	int32 LastWaveNum = 0;

private:
	UPROPERTY()
	class ACoopGameState* CoopGameState;

	UPROPERTY(EditAnywhere, Category = "Spectator")
	TSubclassOf<APawn> SpectatorPawnClass;

	UPROPERTY(EditAnywhere, Category = "DataTable")
	UDataTable* WaveDataTable;

	bool bGameOver;

	float PlayStartTime;
	float PlayEndTime;
	bool bIsPlayTimeEnded;
	bool bTimeRecordStarted;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void AddSpawnedBotNum() { SpawnedBotNum++; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void SubSpawnedBotNum() { SpawnedBotNum--; }
	FORCEINLINE int32 GetSpawnedBotNum() const { return SpawnedBotNum; }
};
