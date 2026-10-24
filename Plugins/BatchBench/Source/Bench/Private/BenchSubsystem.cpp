#include "BenchSubsystem.h"
#include "BenchCVars.h"
#include "Engine/World.h"

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

	ActiveMode = static_cast<EBenchMode>(FMath::Clamp(BenchCVars::Mode.GetValueOnGameThread(), 0, static_cast<int32>(EBenchMode::Count) - 1));

	UE_LOG(LogBench, Display, TEXT("[Bench] subsystem initialized: world=%s mode=%s count=%d moving=%d"),
		*GetWorld()->GetName(),
		Bench::ModeToString(ActiveMode),
		BenchCVars::Count.GetValueOnGameThread(),
		BenchCVars::Moving.GetValueOnGameThread());
}

void UBenchSubsystem::Deinitialize()
{
	BenchCVars::Mode->AsVariable()->OnChangedDelegate().Remove(ModeHandle);
	BenchCVars::Count->AsVariable()->OnChangedDelegate().Remove(CountHandle);
	BenchCVars::Moving->AsVariable()->OnChangedDelegate().Remove(MovingHandle);
	BenchCVars::Record->AsVariable()->OnChangedDelegate().Remove(RecordHandle);

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
	const int32 Raw = Var->GetInt();
	if (Raw < 0 || Raw >= static_cast<int32>(EBenchMode::Count))
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] lab.Mode=%d out of range (0..%d)"), Raw, static_cast<int32>(EBenchMode::Count) - 1);
		return;
	}

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
	UE_LOG(LogBench, Display, TEXT("[Bench] count -> %d"), Var->GetInt());
	ApplySettings();
}

void UBenchSubsystem::OnMovingChanged(IConsoleVariable* Var)
{
	const bool bMoving = Var->GetInt() != 0;
	UE_LOG(LogBench, Display, TEXT("[Bench] moving -> %d"), bMoving ? 1 : 0);
	if (IBatchSpawner* Spawner = FindSpawner(ActiveMode))
	{
		Spawner->SetMoving(bMoving);
	}
}

void UBenchSubsystem::OnRecordChanged(IConsoleVariable* Var)
{
	// 2단계(BenchRecorder)에서 구현.
	UE_LOG(LogBench, Display, TEXT("[Bench] record -> %d (recorder not implemented yet)"), Var->GetInt());
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

	UE_LOG(LogBench, Display, TEXT("[Bench] applied: %s x%d moving=%d (alive=%d)"),
		Bench::ModeToString(ActiveMode), Count, bMoving ? 1 : 0, Spawner->GetCount());
}
