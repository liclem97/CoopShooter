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

// 플레이어를 리스폰하고 관전자 폰을 파괴한다.
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

	// 맵에 있는 모든 SpectatorPawn을 찾고 파괴한다.
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
			// 웨이브에 따른 봇 소환. 블루프린트에 정의되어 있음.
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaitingToComplete)
		{
			CheckWaveState(); // 살아있는 봇이 있나 확인.
		}
		else if (CoopGameState->GetWaveState() == EWaveState::WaveComplete)
		{	
			PlayerRespawnAndDestroySpectator(); // 죽은 플레이어 리스폰 및 관찰자 폰 삭제.
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
	// 메시지 초기화
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
		// 게임 클리어
		SetWaveState(EWaveState::GameOver);
	}
	
}

void ACoopGameMode::SetSpawnEnemyNum()
{	
	if (WaveDataTable)
	{	
		// 테이블에 있는 스폰할 적의 수를 읽어서 저장한다.
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
		// 게임이 시작된 시간을 기록한다.
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
		// 게임이 끝난 시간을 기록한다.
		PlayEndTime = GetWorld()->GetTimeSeconds();
		bIsPlayTimeEnded = true;
	}
}

float ACoopGameMode::GetPlayTime() const
{	
	if (PlayStartTime > 0.f)
	{	
		// 총 플레이타임을 계산한다.
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
	// 봇 스폰 타이머를 초기화하고 다음 상태로 넘어간다.
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	SetWaveState(EWaveState::WaitingToComplete);
}

void ACoopGameMode::CheckWaveState()
{	
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);

	if (NumOfBotsToSpawn > 0 || bIsPreparingForWave) return;
	bool bIsAnyBotAlive = false;

	// 맵에 있는 모든 폰을 확인한다.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* CheckedPawn = *It;
		// 플레이어 폰이면 넘어간다.
		if (CheckedPawn == nullptr || CheckedPawn->IsPlayerControlled()) continue;

		// 감지된 폰이 플레이어가 아닌 경우 멈춤.
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
				// 플레이어가 살아있음.
				return;
			}
		}
	}
	
	// 플레이어가 전부 죽음.
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
		// 봇 스포너 타이머가 실행 중이면 타이머 클리어.
		GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	}	

	// 맵에 있는 모든 플레이어에게 게임오버 위젯을 띄운다.
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

	// 플레이어가 2명이 되면 게임 시작.
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

	//플레이어가 봇을 죽였을 시 1점 추가
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && CoopGameState)
	{
		AttackerPlayerState->AddToKill(1.f);
	}

	//플레이어가 봇에게 죽었을 시 Death 1점 추가 및 사망 메시지
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

			// 게임 모드의 디폴트 폰 클래스를 가져온다.
			UClass* DefaultPawn = GetDefaultPawnClassForController(ElimmedController);

			if (DefaultPawn)
			{
				// 새로운 폰을 스폰하고 컨트롤러를 빙의
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
