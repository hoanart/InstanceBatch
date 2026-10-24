#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BenchTypes.h"
#include "IBatchSpawner.h"
#include "BenchSubsystem.generated.h"

class IConsoleVariable;

/**
 * 벤치 제어 진입점. lab.* CVar 변화를 받아 활성 스포너를 바꾼다.
 * 게임/PIE 월드에서만 생성된다 (에디터 월드에서 수만 개가 스폰되는 사고 방지).
 *
 * 1단계: CVar 수신·로그·스포너 등록 구조만. 측정(BenchRecorder)은 2단계.
 */
UCLASS()
class BENCH_API UBenchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 방식별 스포너를 등록한다. 같은 모드로 다시 등록하면 교체. */
	void RegisterSpawner(EBenchMode Mode, TScriptInterface<IBatchSpawner> Spawner);

	EBenchMode GetActiveMode() const { return ActiveMode; }

private:
	void OnModeChanged(IConsoleVariable* Var);
	void OnCountChanged(IConsoleVariable* Var);
	void OnMovingChanged(IConsoleVariable* Var);
	void OnRecordChanged(IConsoleVariable* Var);

	/** 현재 CVar 값으로 활성 스포너를 다시 구성한다. */
	void ApplySettings();

	IBatchSpawner* FindSpawner(EBenchMode Mode) const;

	UPROPERTY()
	TMap<EBenchMode, TScriptInterface<IBatchSpawner>> Spawners;

	EBenchMode ActiveMode = EBenchMode::Actor;

	FDelegateHandle ModeHandle;
	FDelegateHandle CountHandle;
	FDelegateHandle MovingHandle;
	FDelegateHandle RecordHandle;
};
