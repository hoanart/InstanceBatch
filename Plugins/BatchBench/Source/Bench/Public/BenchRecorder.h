#pragma once

#include "CoreMinimal.h"
#include "BenchTypes.h"

/** 측정 1회의 조건. CSV 한 줄의 앞쪽 컬럼이 된다. */
struct FBenchRunInfo
{
	EBenchMode Mode = EBenchMode::Actor;
	int32 Count = 0;
	int32 Alive = 0;
	bool bMoving = false;
	FString Tag;	// sweep 묶음 식별자. 단발 측정이면 비어 있음
};

/** 측정 1회의 결과 (전부 ms, 샘플 구간 평균). */
struct FBenchResult
{
	int32 Frames = 0;
	double FrameMs = 0.0;
	double FrameMaxMs = 0.0;
	double GameMs = 0.0;
	double RenderMs = 0.0;
	double RhiMs = 0.0;
	double GpuMs = 0.0;
	int32 GpuZeroFrames = 0;	// GPU 값이 0으로 들어온 프레임 수 (GPU 타이밍 비활성 감지용)
};

/**
 * 워밍업 N프레임을 버린 뒤 N프레임 평균을 내서 CSV에 한 줄 기록한다.
 *
 * 값의 출처는 stat unit과 동일:
 *   Game/Render/RHI = GGameThreadTime / GRenderThreadTime / GRHIThreadTime (대기 제외 작업 시간)
 *   GPU             = RHIGetGPUFrameCycles()
 * 전부 사이클 단위라 FPlatformTime::ToMilliseconds로 변환.
 */
class BENCH_API FBenchRecorder
{
public:
	void Start(const FBenchRunInfo& InInfo, int32 InWarmupFrames, int32 InSampleFrames);
	void Cancel();

	/** 매 프레임 호출. 측정이 끝난 프레임에 true를 돌려준다. */
	bool Tick(float DeltaSeconds);

	bool IsRecording() const { return State != EState::Idle; }
	const FBenchResult& GetLastResult() const { return Result; }

	static FString GetCsvPath();

private:
	enum class EState : uint8 { Idle, Warmup, Sampling };

	void Finish();
	void WriteCsvRow() const;

	EState State = EState::Idle;
	FBenchRunInfo Info;
	int32 WarmupFrames = 60;
	int32 SampleFrames = 60;
	int32 FramesLeft = 0;

	// 합계 누적. Finish에서 평균으로 바꾼다.
	double SumFrameMs = 0.0;
	double SumGameMs = 0.0;
	double SumRenderMs = 0.0;
	double SumRhiMs = 0.0;
	double SumGpuMs = 0.0;
	double MaxFrameMs = 0.0;
	int32 Sampled = 0;
	int32 GpuZero = 0;

	FBenchResult Result;
};
