// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "CoopSpectatorPawn.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerLeftGame);

/**
 * 
 */
UCLASS()
class COOPSHOOTER_API ACoopSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()
	
public:
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	void Elim(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	FPlayerLeftGame OnLeftGame;

protected:

private:
	class ACoopGameMode* CoopGameMode;
	class ACoopPlayerState* CoopPlayerState;

	bool bLeftGame = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();
};
