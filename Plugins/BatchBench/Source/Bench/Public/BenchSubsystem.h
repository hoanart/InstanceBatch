#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BenchTypes.h"
#include "BenchRecorder.h"
#include "IBatchSpawner.h"
#include "BenchSubsystem.generated.h"

class IConsoleVariable;
class ACameraActor;
class UStaticMesh;

/**
 * 벤치 제어 진입점. lab.* CVar 변화를 받아 활성 스포너를 바꾸고, 측정기를 돌린다.
 * 게임/PIE 월드에서만 생성된다 (에디터 월드에서 수만 개가 스폰되는 사고 방지).
 */
UCLASS()
class BENCH_API UBenchSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** 방식별 스포너를 등록한다. 같은 모드로 다시 등록하면 교체. */
	void RegisterSpawner(EBenchMode Mode, TScriptInterface<IBatchSpawner> Spawner);

	EBenchMode GetActiveMode() const { return ActiveMode; }

private:
	void OnModeChanged(IConsoleVariable* Var);
	void OnCountChanged(IConsoleVariable* Var);
	void OnMovingChanged(IConsoleVariable* Var);
	void OnRecordChanged(IConsoleVariable* Var);
	void OnSweepChanged(IConsoleVariable* Var);

	/** 현재 CVar 값으로 활성 스포너를 다시 구성한다. */
	void ApplySettings();

	void StartRecording();

	/** 자동 측정. 같은 (모드, 개수) 안에서는 재스폰 없이 정지 -> 이동만 전환한다. */
	struct FSweepStep
	{
		EBenchMode Mode = EBenchMode::Actor;
		int32 Count = 0;
		bool bMoving = false;
	};
	void StartSweep();
	void CancelSweep(const TCHAR* Reason);
	void RunSweepStep();
	bool IsSweeping() const { return SweepIndex >= 0; }

	/** 자체 CVar 설정 중에는 콜백 무시 (sweep이 CVar를 맞춰둘 때 스스로를 취소하지 않도록). */
	void SetCVarSilently(IConsoleVariable* Var, int32 Value);

	/** 방식별 스포너 생성·등록. OnWorldBeginPlay에서 1회. */
	void CreateSpawners(UWorld& InWorld);

	/** 고정 카메라. 측정마다 화면 구성이 같아야 GPU 비교가 의미 있다. */
	void EnsureBenchCamera();

	IBatchSpawner* FindSpawner(EBenchMode Mode) const;

	UPROPERTY()
	TMap<EBenchMode, TScriptInterface<IBatchSpawner>> Spawners;

	EBenchMode ActiveMode = EBenchMode::Actor;

	FBenchRecorder Recorder;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BenchMesh;

	UPROPERTY()
	TObjectPtr<ACameraActor> BenchCamera;

	bool bViewTargetSet = false;

	FDelegateHandle ModeHandle;
	FDelegateHandle CountHandle;
	FDelegateHandle MovingHandle;
	FDelegateHandle RecordHandle;
	FDelegateHandle SweepHandle;

	TArray<FSweepStep> SweepSteps;
	int32 SweepIndex = -1;
	FString SweepTag;
	double SweepStartTime = 0.0;

	/** 현재 실제로 스폰돼 있는 조건 (sweep에서 재스폰이 필요한지 판단). */
	EBenchMode SpawnedMode = EBenchMode::Count;
	int32 SpawnedCount = -1;

	bool bSuppressCVarCallbacks = false;
};
