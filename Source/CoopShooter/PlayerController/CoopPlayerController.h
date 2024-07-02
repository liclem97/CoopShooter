// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CoopShooter/Weapon/WeaponTypes.h"
#include "CoopPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class COOPSHOOTER_API ACoopPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDKillScore(float KillScore);
	void SetHUDDeathScore(int32 Death);
	void SetHUDMessage(FString Message);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDPlayTime(float PlayTime);
	void SetHUDWeaponType(EWeaponType NewWeaponType);
	void SetHUDPlayingState(FString PlayingState);
	void SetHUDSpawnedBotNum(int32 BotNum);
	void AddGameOverWidget(float PlayTime, int32 ClearedWave, int32 Kill, int32 Death);
	void RemoveCharacterOverlay();
	void SetGameoverHUDAnnouncement(int32 CountdownTime);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // 서버 시계와 동기화;
	virtual void ReceivedPlayer() override; // 서버 시계와 가능한 동기화

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	bool bIsSpectator = false;

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();
	virtual void SetupInputComponent() override;

	/**
	* 클라이언트와 서버 사이 시간 동기화
	*/
	// 요청이 전송된 클라이언트 시간을 기준으로 현재 서버 시간을 요청합니다.
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Server RequestServerTime에 대한 응답으로 현재 서버 시간을 클라이언트에 보고합니다.
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // 클라이언트 시간과 서버 시간의 차이.

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);
	void HighPingWarnig();
	void StopHighPingWarnig();
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();

private:
	UPROPERTY()
	class ACoopHUD* CoopHUD;

	/**
	* Return to main menu
	*/
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool  bReturnToMainMenuOpen = false;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	class UGameOver* GameOver;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDKillScore;
	bool bInitializeKillScore = false;
	int32 HUDDeath;
	bool bInitializeDeath = false;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;
	EWeaponType HUDWeaponType;
	bool bInitializeWeaponType = false;
	FString HUDPlayingState;
	bool bInitializePlayingState = false;
	int32 HUDBotNum;
	bool bInitializeBotNum = false;
	int32 HUDCountdownTime;
	bool bInitializeCountdown = false;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

	bool bTickingTime = false;

public:
	FORCEINLINE ACoopHUD* GetCoopHUD() const { return CoopHUD; }
};
