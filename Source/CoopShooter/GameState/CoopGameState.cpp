// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopGameState.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACoopGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACoopGameState, WaveState);
}

void ACoopGameState::SetWaveState(EWaveState NewState)
{
	if (HasAuthority())
	{	
		EWaveState OldState = WaveState;

		WaveState = NewState;
		OnRep_WaveState(OldState);
	}
}

EWaveState ACoopGameState::GetWaveState()
{
	return WaveState;
}

void ACoopGameState::OnRep_WaveState(EWaveState OldState)
{	
	WaveStateChanged(WaveState, OldState);
}
