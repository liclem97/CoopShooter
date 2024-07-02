// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOver.generated.h"

/**
 * 
 */
UCLASS()
class COOPSHOOTER_API UGameOver : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativePreConstruct();

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayTimeText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ClearWaveText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DeathText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;

	UPROPERTY(meta = (BindWidget))
	class UButton* GoToMainMenuButton;

	UFUNCTION()
	void GoToMainMenuButtonClicked();

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeftGame();

private:
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY()
	class APlayerController* PlayerController;
};
