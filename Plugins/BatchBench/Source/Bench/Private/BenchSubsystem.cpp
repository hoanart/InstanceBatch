#include "BenchSubsystem.h"
#include "BenchCVars.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "BenchLayout.h"
#include "Spawners/BenchActorSpawner.h"
#include "Spawners/BenchISMSpawner.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

bool UBenchSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (!World)
	{
		return false;
	}
	// 에디터 프리뷰/에디터 월드는 제외. 게임과 PIE에서만 동작.
	return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
}

void UBenchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ModeHandle   = BenchCVars::Mode->AsVariable()->OnChangedDelegate().AddUObject(this, &UBenchSubsystem::OnModeChanged);
	CountHandle  = BenchCVars::Count->AsVariable()->OnChangedDelegate().AddUObject(this, &UBenchSubsystem::OnCountChanged);
	MovingHandle = BenchCVars::Moving->AsVariable()->OnChangedDelegate().AddUObject(this, &UBenchSubsystem::OnMovingChanged);
	RecordHandle = BenchCVars::Record->AsVariable()->OnChangedDelegate().AddUObject(this, &UBenchSubsystem::OnRecordChanged);
	SweepHandle  = BenchCVars::Sweep->AsVariable()->OnChangedDelegate().AddUObject(this, &UBenchSubsystem::OnSweepChanged);

	ActiveMode = static_cast<EBenchMode>(FMath::Clamp(BenchCVars::Mode.GetValueOnGameThread(), 0, static_cast<int32>(EBenchMode::Count) - 1));

	UE_LOG(LogBench, Display, TEXT("[Bench] subsystem initialized: world=%s mode=%s count=%d moving=%d"),
		*GetWorld()->GetName(),
		Bench::ModeToString(ActiveMode),
		BenchCVars::Count.GetValueOnGameThread(),
		BenchCVars::Moving.GetValueOnGameThread());
}

void UBenchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CreateSpawners(InWorld);
	EnsureBenchCamera();

	// 시작 시 현재 CVar 값으로 한 번 구성한다 (기본: Actor x1000 정지).
	// 빈 씬 기준값이 필요하면 lab.Count 0.
	ApplySettings();
}

void UBenchSubsystem::CreateSpawners(UWorld& InWorld)
{
	// 엔진 기본 큐브(12 tris). 정점 비용을 최소로 두어 객체 수에 따른 CPU/드로우 비용이 잘 드러나게 한다.
	BenchMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!BenchMesh)
	{
		UE_LOG(LogBench, Error, TEXT("[Bench] failed to load /Engine/BasicShapes/Cube"));
		return;
	}

	UBenchActorSpawner* ActorSpawner = NewObject<UBenchActorSpawner>(this);
	ActorSpawner->Setup(&InWorld, BenchMesh);
	RegisterSpawner(EBenchMode::Actor, ActorSpawner);

	UBenchISMSpawner* ISMDirtyEach = NewObject<UBenchISMSpawner>(this);
	ISMDirtyEach->Setup(&InWorld, BenchMesh, EISMUpdate::DirtyEach);
	RegisterSpawner(EBenchMode::ISM_DirtyEach, ISMDirtyEach);

	UBenchISMSpawner* ISMDirtyOnce = NewObject<UBenchISMSpawner>(this);
	ISMDirtyOnce->Setup(&InWorld, BenchMesh, EISMUpdate::DirtyOnce);
	RegisterSpawner(EBenchMode::ISM_DirtyOnce, ISMDirtyOnce);

	// 위치 = 기준 위치 + (0,0, 50 * sin(2t + CustomData0)) 을 머티리얼 WPO가 계산 (BenchLayout과 같은 식).
	UMaterialInterface* MoveMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Bench/M_Bench_ShaderMove.M_Bench_ShaderMove"));
	if (MoveMat)
	{
		UBenchISMSpawner* ISMShader = NewObject<UBenchISMSpawner>(this);
		ISMShader->Setup(&InWorld, BenchMesh, EISMUpdate::ShaderMove, MoveMat);
		RegisterSpawner(EBenchMode::ISM_ShaderMove, ISMShader);
	}
	else
	{
		UE_LOG(LogBench, Error, TEXT("[Bench] failed to load /Game/Bench/M_Bench_ShaderMove - ShaderMove mode disabled"));
	}
}

void UBenchSubsystem::EnsureBenchCamera()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!BenchCamera)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		BenchCamera = World->SpawnActor<ACameraActor>(Bench::Layout::GetCameraLocation(), Bench::Layout::GetCameraRotation(), Params);
		if (BenchCamera)
		{
			BenchCamera->GetCameraComponent()->SetFieldOfView(90.f);
			BenchCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
		}
	}

	if (!bViewTargetSet && BenchCamera)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetViewTarget(BenchCamera);
			bViewTargetSet = true;
			UE_LOG(LogBench, Display, TEXT("[Bench] view target -> fixed bench camera"));
		}
	}
}

void UBenchSubsystem::Deinitialize()
{
	SweepIndex = -1;
	Recorder.Cancel();

	BenchCVars::Mode->AsVariable()->OnChangedDelegate().Remove(ModeHandle);
	BenchCVars::Count->AsVariable()->OnChangedDelegate().Remove(CountHandle);
	BenchCVars::Moving->AsVariable()->OnChangedDelegate().Remove(MovingHandle);
	BenchCVars::Record->AsVariable()->OnChangedDelegate().Remove(RecordHandle);
	BenchCVars::Sweep->AsVariable()->OnChangedDelegate().Remove(SweepHandle);

	for (TPair<EBenchMode, TScriptInterface<IBatchSpawner>>& Pair : Spawners)
	{
		if (IBatchSpawner* Spawner = Pair.Value.GetInterface())
		{
			Spawner->Clear();
		}
	}
	Spawners.Empty();

	Super::Deinitialize();
}

void UBenchSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// PlayerController가 BeginPlay 이후에 생기는 경우를 대비해 뷰 타깃을 지연 설정.
	if (!bViewTargetSet)
	{
		EnsureBenchCamera();
	}

	// 중앙에서 이동을 처리하는 방식(ISM 등)의 갱신. 이 비용은 게임 스레드 시간에 포함된다.
	if (IBatchSpawner* Spawner = FindSpawner(ActiveMode))
	{
		Spawner->TickSpawner(DeltaTime, GetWorld()->GetTimeSeconds());
	}

	// 월드 DeltaTime은 time dilation이 걸리면 실제 프레임 시간과 달라진다.
	// 측정은 실제 벽시계 프레임 시간이어야 하므로 FApp::GetDeltaTime()을 쓴다.
	if (Recorder.Tick(static_cast<float>(FApp::GetDeltaTime())))
	{
		if (IsSweeping())
		{
			++SweepIndex;
			RunSweepStep();
			return;
		}

		// 측정 완료 → lab.Record를 0으로 되돌린다.
		// SetByConsole로 설정해야 콘솔에서 넣은 값보다 우선순위가 낮아 무시되는 일이 없다.
		// Recorder는 이미 Idle이라 OnRecordChanged(0)은 아무 것도 하지 않는다.
		BenchCVars::Record->AsVariable()->Set(0, ECVF_SetByConsole);
	}
}

TStatId UBenchSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBenchSubsystem, STATGROUP_Tickables);
}

void UBenchSubsystem::RegisterSpawner(EBenchMode Mode, TScriptInterface<IBatchSpawner> Spawner)
{
	Spawners.Add(Mode, Spawner);
	UE_LOG(LogBench, Display, TEXT("[Bench] spawner registered: %s"), Bench::ModeToString(Mode));
}

IBatchSpawner* UBenchSubsystem::FindSpawner(EBenchMode Mode) const
{
	const TScriptInterface<IBatchSpawner>* Found = Spawners.Find(Mode);
	return Found ? Found->GetInterface() : nullptr;
}

void UBenchSubsystem::OnModeChanged(IConsoleVariable* Var)
{
	if (bSuppressCVarCallbacks)
	{
		return;
	}
	CancelSweep(TEXT("manual change"));
	const int32 Raw = Var->GetInt();
	if (Raw < 0 || Raw >= static_cast<int32>(EBenchMode::Count))
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] lab.Mode=%d out of range (0..%d)"), Raw, static_cast<int32>(EBenchMode::Count) - 1);
		return;
	}

	// 측정 중에 조건이 바뀌면 그 측정은 무효.
	Recorder.Cancel();

	// 이전 방식의 객체를 먼저 지워야 두 방식이 섞여 측정되지 않는다.
	if (IBatchSpawner* Prev = FindSpawner(ActiveMode))
	{
		Prev->Clear();
	}

	ActiveMode = static_cast<EBenchMode>(Raw);
	UE_LOG(LogBench, Display, TEXT("[Bench] mode -> %s"), Bench::ModeToString(ActiveMode));
	ApplySettings();
}

void UBenchSubsystem::OnCountChanged(IConsoleVariable* Var)
{
	if (bSuppressCVarCallbacks)
	{
		return;
	}
	CancelSweep(TEXT("manual change"));
	Recorder.Cancel();
	UE_LOG(LogBench, Display, TEXT("[Bench] count -> %d"), Var->GetInt());
	ApplySettings();
}

void UBenchSubsystem::OnMovingChanged(IConsoleVariable* Var)
{
	if (bSuppressCVarCallbacks)
	{
		return;
	}
	CancelSweep(TEXT("manual change"));
	Recorder.Cancel();
	const bool bMoving = Var->GetInt() != 0;
	UE_LOG(LogBench, Display, TEXT("[Bench] moving -> %d"), bMoving ? 1 : 0);
	if (IBatchSpawner* Spawner = FindSpawner(ActiveMode))
	{
		Spawner->SetMoving(bMoving);
	}
}

void UBenchSubsystem::OnRecordChanged(IConsoleVariable* Var)
{
	if (bSuppressCVarCallbacks)
	{
		return;
	}
	if (IsSweeping())
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] sweep in progress, lab.Record ignored"));
		return;
	}
	if (Var->GetInt() != 0)
	{
		if (Recorder.IsRecording())
		{
			UE_LOG(LogBench, Warning, TEXT("[Bench] already recording"));
			return;
		}
		StartRecording();
	}
	else
	{
		Recorder.Cancel();	// Idle이면 아무 일도 안 함
	}
}

void UBenchSubsystem::StartRecording()
{
	FBenchRunInfo Info;
	Info.Mode    = ActiveMode;
	Info.Count   = BenchCVars::Count.GetValueOnGameThread();
	Info.bMoving = BenchCVars::Moving.GetValueOnGameThread() != 0;

	// 스포너가 없으면 alive=0 그대로 → 빈 씬 기준값(baseline) 측정이 된다.
	if (const IBatchSpawner* Spawner = FindSpawner(ActiveMode))
	{
		Info.Alive = Spawner->GetCount();
	}

	Recorder.Start(Info,
		BenchCVars::WarmupFrames.GetValueOnGameThread(),
		BenchCVars::SampleFrames.GetValueOnGameThread());
}

void UBenchSubsystem::ApplySettings()
{
	IBatchSpawner* Spawner = FindSpawner(ActiveMode);
	if (!Spawner)
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] no spawner registered for %s"), Bench::ModeToString(ActiveMode));
		return;
	}

	const int32 Count = FMath::Max(0, BenchCVars::Count.GetValueOnGameThread());
	const bool bMoving = BenchCVars::Moving.GetValueOnGameThread() != 0;

	Spawner->Spawn(Count);
	Spawner->SetMoving(bMoving);
	SpawnedMode = ActiveMode;
	SpawnedCount = Count;

	UE_LOG(LogBench, Display, TEXT("[Bench] applied: %s x%d moving=%d (alive=%d)"),
		Bench::ModeToString(ActiveMode), Count, bMoving ? 1 : 0, Spawner->GetCount());
}

// ---------------------------------------------------------------------------
// Sweep
// ---------------------------------------------------------------------------

void UBenchSubsystem::SetCVarSilently(IConsoleVariable* Var, int32 Value)
{
	TGuardValue<bool> Guard(bSuppressCVarCallbacks, true);
	Var->Set(Value, ECVF_SetByConsole);
}

void UBenchSubsystem::OnSweepChanged(IConsoleVariable* Var)
{
	if (bSuppressCVarCallbacks)
	{
		return;
	}
	if (Var->GetInt() != 0)
	{
		StartSweep();
	}
	else
	{
		CancelSweep(TEXT("lab.Sweep 0"));
	}
}

void UBenchSubsystem::StartSweep()
{
	if (IsSweeping())
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] sweep already running"));
		return;
	}

	auto ParseInts = [](const FString& In)
	{
		TArray<FString> Parts;
		In.ParseIntoArray(Parts, TEXT(","), true);
		TArray<int32> Out;
		for (FString& P : Parts)
		{
			P.TrimStartAndEndInline();
			if (P.IsNumeric())
			{
				Out.Add(FCString::Atoi(*P));
			}
		}
		return Out;
	};

	const TArray<int32> Modes = ParseInts(BenchCVars::SweepModes.GetValueOnGameThread());
	const TArray<int32> Counts = ParseInts(BenchCVars::SweepCounts.GetValueOnGameThread());

	// 순서: 개수 -> 모드 -> (정지, 이동). 같은 (모드, 개수)에서 이동 전환은 재스폰이 필요 없다.
	SweepSteps.Reset();
	for (int32 Count : Counts)
	{
		for (int32 M : Modes)
		{
			if (M < 0 || M >= static_cast<int32>(EBenchMode::Count) || !FindSpawner(static_cast<EBenchMode>(M)))
			{
				UE_LOG(LogBench, Warning, TEXT("[Bench] sweep: mode %d has no spawner, skipped"), M);
				continue;
			}
			for (int32 Mv = 0; Mv <= 1; ++Mv)
			{
				SweepSteps.Add({ static_cast<EBenchMode>(M), Count, Mv != 0 });
			}
		}
	}

	if (SweepSteps.IsEmpty())
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] sweep: nothing to run"));
		SetCVarSilently(BenchCVars::Sweep->AsVariable(), 0);
		return;
	}

	Recorder.Cancel();
	SweepTag = FString::Printf(TEXT("sweep_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	SweepStartTime = FPlatformTime::Seconds();
	SweepIndex = 0;

	UE_LOG(LogBench, Display, TEXT("[Bench] sweep start: %s, %d steps (modes=%s counts=%s)"),
		*SweepTag, SweepSteps.Num(),
		*BenchCVars::SweepModes.GetValueOnGameThread(), *BenchCVars::SweepCounts.GetValueOnGameThread());

	RunSweepStep();
}

void UBenchSubsystem::CancelSweep(const TCHAR* Reason)
{
	if (!IsSweeping())
	{
		return;
	}
	UE_LOG(LogBench, Warning, TEXT("[Bench] sweep cancelled at %d/%d (%s)"), SweepIndex, SweepSteps.Num(), Reason);
	SweepIndex = -1;
	Recorder.Cancel();
	SetCVarSilently(BenchCVars::Sweep->AsVariable(), 0);
}

void UBenchSubsystem::RunSweepStep()
{
	if (SweepIndex >= SweepSteps.Num())
	{
		UE_LOG(LogBench, Display, TEXT("[Bench] sweep done: %s, %d steps in %.1f s -> %s"),
			*SweepTag, SweepSteps.Num(), FPlatformTime::Seconds() - SweepStartTime,
			*FPaths::ConvertRelativePathToFull(FBenchRecorder::GetCsvPath()));
		SweepIndex = -1;
		SetCVarSilently(BenchCVars::Sweep->AsVariable(), 0);
		return;
	}

	const FSweepStep& Step = SweepSteps[SweepIndex];

	// CVar도 실제 상태에 맞춰둔다 (콜백은 억제).
	SetCVarSilently(BenchCVars::Mode->AsVariable(), static_cast<int32>(Step.Mode));
	SetCVarSilently(BenchCVars::Count->AsVariable(), Step.Count);
	SetCVarSilently(BenchCVars::Moving->AsVariable(), Step.bMoving ? 1 : 0);

	IBatchSpawner* Spawner = FindSpawner(Step.Mode);
	if (!Spawner)
	{
		++SweepIndex;
		RunSweepStep();
		return;
	}

	const bool bNeedRespawn = (SpawnedMode != Step.Mode) || (SpawnedCount != Step.Count);
	if (bNeedRespawn)
	{
		// 이전 방식의 객체를 지우고, 그 쓰레기를 측정 전에 치워야 측정 중 GC 스파이크가 안 섞인다.
		if (SpawnedMode != EBenchMode::Count)
		{
			if (IBatchSpawner* Prev = FindSpawner(SpawnedMode))
			{
				Prev->Clear();
			}
		}
		if (GEngine)
		{
			GEngine->ForceGarbageCollection(true);
		}

		ActiveMode = Step.Mode;
		Spawner->Spawn(Step.Count);
		SpawnedMode = Step.Mode;
		SpawnedCount = Step.Count;
	}
	ActiveMode = Step.Mode;
	Spawner->SetMoving(Step.bMoving);

	FBenchRunInfo Info;
	Info.Mode = Step.Mode;
	Info.Count = Step.Count;
	Info.Alive = Spawner->GetCount();
	Info.bMoving = Step.bMoving;
	Info.Tag = SweepTag;

	UE_LOG(LogBench, Display, TEXT("[Bench] sweep %d/%d: %s x%d moving=%d%s"),
		SweepIndex + 1, SweepSteps.Num(), Bench::ModeToString(Step.Mode), Step.Count, Step.bMoving ? 1 : 0,
		bNeedRespawn ? TEXT(" (respawn)") : TEXT(""));

	Recorder.Start(Info,
		BenchCVars::WarmupFrames.GetValueOnGameThread(),
		BenchCVars::SampleFrames.GetValueOnGameThread());
}
