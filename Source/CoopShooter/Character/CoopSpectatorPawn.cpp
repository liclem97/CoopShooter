// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopSpectatorPawn.h"

#include "CoopShooter/GameMode/CoopGameMode.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"

void ACoopSpectatorPawn::ServerLeaveGame_Implementation()
{	
	CoopGameMode = CoopGameMode == nullptr ? GetWorld()->GetAuthGameMode<ACoopGameMode>() : CoopGameMode;
	CoopPlayerState = CoopPlayerState == nullptr ? GetPlayerState<ACoopPlayerState>() : CoopPlayerState;
	if (CoopGameMode && CoopPlayerState)
	{	
		CoopGameMode->PlayerLeftGame(CoopPlayerState);
	}
}

void ACoopSpectatorPawn::Elim(bool bPlayerLeftGame)
{
	MulticastElim(bPlayerLeftGame);
}

void ACoopSpectatorPawn::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ACoopSpectatorPawn::ElimTimerFinished,
		ElimDelay
	);
}

void ACoopSpectatorPawn::ElimTimerFinished()
{
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}