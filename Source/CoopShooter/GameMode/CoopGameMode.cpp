// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopGameMode.h"

#include "CoopShooter/Character/CoopCharacter.h"
#include "CoopShooter/Character/CoopCharacterBase.h"
#include "CoopShooter/Character/CoopSpectatorPawn.h"
#include "CoopShooter/GameState/CoopGameState.h"
#include "CoopShooter/PlayerController/CoopPlayerController.h"
#include "CoopShooter/PlayerState/CoopPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ACoopGameMode::ACoopGameMode()
{
	PrimaryActorTick.TickInterval = 1.0f;
	bDelayedStart = true;
	TimeBetweenWaves = 5.f;
	WaveCount = 0;
	CountdownTime = 0.f;
	SpawnedBotNum = 0;
	CurrentBotNum = -1;
	bGameOver = false;
	PlayStartTime = 0.f;
	PlayEndTime = 0.f;
	bIsPlayTimeEnded = false;
	bTimeRecordStarted = false;
	CountdownTime = 60;
}

// �÷��̾ �������ϰ� ������ ���� �ı��Ѵ�.
void ACoopGameMode::PlayerRespawnAndDestroySpectator()
{	
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;
		if (Pawn && Pawn->IsA(SpectatorPawnClass))
		{	
			AController* Controller = Pawn->GetController();
			if (Controller)
			{
				RequestRespawn(nullptr, Controller);
			}			
		}
	}

	// �ʿ� �ִ� ��� SpectatorPawn�� ã�� �ı��Ѵ�.
	TArray<AActor*> FoundSpectatorPawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), SpectatorPawnClass, FoundSpectatorPawns);

	for (AActor* SpectatorPawn : FoundSpectatorPawns)
	{
		if (SpectatorPawn)
		{
			SpectatorPawn->Destroy();
		}
	}
	SetWaveState(EWaveState::WaitingToStart);
}

void ACoopGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ACoopGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckAnyPlayerAlive();
	BotNumChanged(SpawnedBotNum);
	CoopGameState = CoopGameState == nullptr ? GetGameState<ACoopGameState>() : CoopGameState;
	if (CoopGameState)
	{	
		UpdatePlayingState(CoopGameState->GetWaveState(), WaveCount);
		if (CoopGameState->GetWaveState() == EWaveState::GameOver)
		{
			EndPlayTime();
			GameOver();			
		}

		if (CoopGameState->GetWaveState() == EWaveState::WaitingPlayer)
		{
			WaitingOtherPlayer();
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaitingToStart)
		{
			StartPlayTime();
			WaitingToStart();
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaveInProgress)
		{
			// ���̺꿡 ���� �� ��ȯ. �������Ʈ�� ���ǵǾ� ����.
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaitingToComplete)
		{
			CheckWaveState(); // ����ִ� ���� �ֳ� Ȯ��.
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaveComplete)
		{	
			PlayerRespawnAndDestroySpectator(); // ���� �÷��̾� ������ �� ������ �� ����.
		}
	}	
}

void ACoopGameMode::WaitingToStart()
{	
	bool bIsWaitingToStart = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);
	if (bIsWaitingToStart) return;
	FString WaveString;
	if (WaveCount < LastWaveNum)
	{
		WaveString = FString::Printf(TEXT("Wave %d will start soon"), WaveCount + 1);
	}
	else
	{
		WaveString = FString::Printf(TEXT("Congratulations! You've cleared all the waves."));
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC)
		{
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{	
				CoopPS->SetServerMessage(WaveString);
			}
		}
	}
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ACoopGameMode::StartWave, TimeBetweenWaves, false);
}

void ACoopGameMode::StartWave()
{	
	// �޽��� �ʱ�ȭ
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC)
		{
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{
				CoopPS->SetServerMessage("");
			}
		}
	}
	WaveCount++;
	if (WaveCount <= LastWaveNum)
	{
		SetSpawnEnemyNum();
		GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ACoopGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.f);

		SetWaveState(EWaveState::WaveInProgress);
	}
	else
	{	
		// ���� Ŭ����
		SetWaveState(EWaveState::GameOver);
	}
	
}

void ACoopGameMode::SetSpawnEnemyNum()
{	
	if (WaveDataTable)
	{	
		// ���̺� �ִ� ������ ���� ���� �о �����Ѵ�.
		FString WaveString = FString::Printf(TEXT("Wave_%d"), WaveCount);
		FString ContextString;
		FWaveData* WaveDataRow = WaveDataTable->FindRow<FWaveData>(FName(*WaveString), ContextString);

		if (WaveDataRow)
		{	
			TrackerBotSpawnNum = WaveDataRow->TrackerBotSpawnNum;
			SGAISpawnNum = WaveDataRow->SGAISpawnNum;
			SRAISpawnNum = WaveDataRow->SRAISpawnNum;
			GLAISpawnNum = WaveDataRow->GLAISpawnNum;
			RLAISpawnNum = WaveDataRow->RLAISpawnNum;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find row with name %s"), *WaveString);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WaveDataTable is not set"));
	}
}

void ACoopGameMode::StartPlayTime()
{	
	if (!bTimeRecordStarted)
	{	
		// ������ ���۵� �ð��� ����Ѵ�.
		PlayStartTime = GetWorld()->GetTimeSeconds();
		bIsPlayTimeEnded = false;
		bTimeRecordStarted = true;
	}
	else return;
}

void ACoopGameMode::EndPlayTime()
{
	if (!bIsPlayTimeEnded)
	{	
		// ������ ���� �ð��� ����Ѵ�.
		PlayEndTime = GetWorld()->GetTimeSeconds();
		bIsPlayTimeEnded = true;
	}
}

float ACoopGameMode::GetPlayTime() const
{	
	if (PlayStartTime > 0.f)
	{	
		// �� �÷���Ÿ���� ����Ѵ�.
		if (bIsPlayTimeEnded) return (PlayEndTime - PlayStartTime);
		else (GetWorld()->GetTimeSeconds() - PlayStartTime);
	}
	return 0.0f;
}

void ACoopGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();
}

void ACoopGameMode::EndWave()
{	
	// �� ���� Ÿ�̸Ӹ� �ʱ�ȭ�ϰ� ���� ���·� �Ѿ��.
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	SetWaveState(EWaveState::WaitingToComplete);
}

void ACoopGameMode::CheckWaveState()
{	
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);

	if (NumOfBotsToSpawn > 0 || bIsPreparingForWave) return;
	bool bIsAnyBotAlive = false;

	// �ʿ� �ִ� ��� ���� Ȯ���Ѵ�.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* CheckedPawn = *It;
		// �÷��̾� ���̸� �Ѿ��.
		if (CheckedPawn == nullptr || CheckedPawn->IsPlayerControlled()) continue;

		// ������ ���� �÷��̾ �ƴ� ��� ����.
		ACoopCharacterBase* CheckedCharacter = Cast<ACoopCharacterBase>(CheckedPawn);
		if (CheckedCharacter && !CheckedCharacter->IsElimmed())
		{
			bIsAnyBotAlive = true;
			break;
		}
	}

	if (!bIsAnyBotAlive)
	{
		SetWaveState(EWaveState::WaveComplete);
	}
}

void ACoopGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			ACoopCharacterBase* CoopCharacter = Cast<ACoopCharacterBase>(PC->GetPawn());
			if (CoopCharacter && !CoopCharacter->IsElimmed())
			{
				// �÷��̾ �������.
				return;
			}
		}
	}
	
	// �÷��̾ ���� ����.
	SetWaveState(EWaveState::GameOver);;
}

void ACoopGameMode::CheckBotNum()
{
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* IteratedPawn = *It;
		if (IteratedPawn == nullptr || IteratedPawn->IsPlayerControlled()) continue;

		ACoopCharacterBase* IteratedCharacterBase = Cast<ACoopCharacterBase>(IteratedPawn);
		if (IteratedCharacterBase && !IteratedCharacterBase->IsElimmed())
		{
			SpawnedBotNum++;
		}
	}
	SpawnedBotNum = 0;
}

void ACoopGameMode::GameOver()
{	
	if (bGameOver) return;
	bGameOver = true;
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_BotSpawner))
	{	
		// �� ������ Ÿ�̸Ӱ� ���� ���̸� Ÿ�̸� Ŭ����.
		GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	}	

	// �ʿ� �ִ� ��� �÷��̾�� ���ӿ��� ������ ����.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC && CoopPC->GetPawn())
		{
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{
				CoopPS->AddGameOverWidget(bGameOver, GetPlayTime(), WaveCount - 1, CountdownTime);
			}
		}
	}
}

void ACoopGameMode::SetWaveState(EWaveState NewState)
{	
	CoopGameState = CoopGameState == nullptr ? GetGameState<ACoopGameState>() : CoopGameState;
	if (CoopGameState)
	{
		CoopGameState->SetWaveState(NewState);
	}
}

void ACoopGameMode::WaitingOtherPlayer()
{	
	int32 NumOfPlayers = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC && CoopPC->GetPawn())
		{	
			++NumOfPlayers;
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{
				CoopPS->SetServerMessage("Waiting for other player");
			}
		}
	}

	// �÷��̾ 2���� �Ǹ� ���� ����.
	if (NumOfPlayers == 2)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
			if (CoopPC)
			{
				SetWaveState(EWaveState::WaitingToStart);
			}
		}
	}
}

void ACoopGameMode::PawnEliminated(ACoopCharacterBase* ElimmedPawn, AController* VictimController, AController* AttackerController)
{	
	ACoopPlayerState* AttackerPlayerState = AttackerController ? Cast<ACoopPlayerState>(AttackerController->PlayerState) : nullptr;
	ACoopPlayerState* VictimPlayerState = VictimController ? Cast<ACoopPlayerState>(VictimController->PlayerState) : nullptr;
	CoopGameState = CoopGameState == nullptr ? GetGameState<ACoopGameState>() : CoopGameState;

	//�÷��̾ ���� �׿��� �� 1�� �߰�
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && CoopGameState)
	{
		AttackerPlayerState->AddToKill(1.f);
	}

	//�÷��̾ ������ �׾��� �� Death 1�� �߰� �� ��� �޽���
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDeath(1);
		VictimPlayerState->SetServerMessage(TEXT("You Were Eliminated!"));
	}

	if (ElimmedPawn)
	{	
		ACoopCharacter* ElimmedCharacter = Cast<ACoopCharacter>(ElimmedPawn);
		if (ElimmedCharacter)
		{
			ElimmedCharacter->Elim(false);
		}
	}
}

void ACoopGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{	
	ACoopCharacter* CoopCharacter = Cast<ACoopCharacter>(ElimmedCharacter);
	if (CoopCharacter)
	{
		CoopCharacter->Reset();
		CoopCharacter->Destroy();
	}
	if (ElimmedController)
	{
		ACoopPlayerState* PlayerState = ElimmedController ? Cast<ACoopPlayerState>(ElimmedController->PlayerState) : nullptr;
		if (PlayerState)
		{
			PlayerState->SetServerMessage(TEXT(""));
		}

		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		if (PlayerStarts.Num() > 0)
		{
			int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
			AActor* ChosenStart = PlayerStarts[Selection];

			// ���� ����� ����Ʈ �� Ŭ������ �����´�.
			UClass* DefaultPawn = GetDefaultPawnClassForController(ElimmedController);

			if (DefaultPawn)
			{
				// ���ο� ���� �����ϰ� ��Ʈ�ѷ��� ����
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = ElimmedController;
				APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawn, ChosenStart->GetActorLocation(), ChosenStart->GetActorRotation(), SpawnParams);
				if (NewPawn)
				{
					ElimmedController->UnPossess();
					ElimmedController->Possess(NewPawn);
				}
			}
		}
	}
}

void ACoopGameMode::UpdateSpawnedBotNum(int32 NewSpawnedBotNum)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC)
		{
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{
				CoopPS->UpdateSpawnedBotNum(NewSpawnedBotNum);
			}
		}
	}
}

void ACoopGameMode::BotNumChanged(int32 NewBotNum)
{
	if (CurrentBotNum == NewBotNum) return;
	else
	{
		CurrentBotNum = NewBotNum;
		UpdateSpawnedBotNum(NewBotNum);
	}	
}

FString ACoopGameMode::GetWaveStateText(EWaveState NewWaveState, int32 NewWaveCount)
{	
	FString PlayingStateText;

	switch (NewWaveState)
	{
	case EWaveState::WaitingPlayer:
		PlayingStateText = WaveStateString::WaitingPlayer;
		break;
	case EWaveState::WaitingToStart:
		PlayingStateText = WaveStateString::WaitingToStart;
		break;
	case EWaveState::WaitingToComplete:
		PlayingStateText = FString::Printf(TEXT("Wave %d"), NewWaveCount);
		break;
	case EWaveState::WaveComplete:
		PlayingStateText = WaveStateString::WaveComplete;
		break;
	case EWaveState::GameOver:
		PlayingStateText = WaveStateString::GameOver;
		break;
	default:
		PlayingStateText = FString::Printf(TEXT(""), NewWaveCount);
		break;
	}

	return PlayingStateText;
}

void ACoopGameMode::UpdatePlayingState(EWaveState NewWaveState, int32 NewWaveCount)
{	
	FString PlayingStateText = GetWaveStateText(NewWaveState, NewWaveCount);
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACoopPlayerController* CoopPC = Cast<ACoopPlayerController>(*It);
		if (CoopPC)
		{
			ACoopPlayerState* CoopPS = Cast<ACoopPlayerState>(CoopPC->PlayerState);
			if (CoopPS)
			{
				CoopPS->UpdatePlayingState(PlayingStateText);
			}
		}
	}
}

void ACoopGameMode::PlayerLeftGame(ACoopPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	ACoopCharacter* CharacterLeaving = Cast<ACoopCharacter>(PlayerLeaving->GetPawn());
	ACoopSpectatorPawn* SpectatorLeaving = Cast<ACoopSpectatorPawn>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
	else if (SpectatorLeaving)
	{
		SpectatorLeaving->Elim(true);
	}
}
