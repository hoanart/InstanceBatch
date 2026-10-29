#include "BenchRecorder.h"

#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "RHIGlobals.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "ProfilingDebugging/MiscTrace.h"

namespace
{
	int32 GetIntCVar(const TCHAR* Name, int32 Fallback = -1)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
		return Var ? Var->GetInt() : Fallback;
	}

	FIntPoint GetViewportSize()
	{
		if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
		{
			return GEngine->GameViewport->Viewport->GetSizeXY();
		}
		return FIntPoint::ZeroValue;
	}

	/** CSV 안에 쉼표가 들어가면 컬럼이 밀리므로 따옴표로 감싼다. */
	FString CsvQuote(const FString& In)
	{
		return FString::Printf(TEXT("\"%s\""), *In.Replace(TEXT("\""), TEXT("'")));
	}
}

FString FBenchRecorder::GetCsvPath()
{
	return FPaths::ProjectSavedDir() / TEXT("Bench") / TEXT("bench_results.csv");
}

void FBenchRecorder::Start(const FBenchRunInfo& InInfo, int32 InWarmupFrames, int32 InSampleFrames)
{
	Info = InInfo;
	WarmupFrames = FMath::Max(0, InWarmupFrames);
	SampleFrames = FMath::Max(1, InSampleFrames);

	SumFrameMs = SumGameMs = SumRenderMs = SumRhiMs = SumGpuMs = 0.0;
	MaxFrameMs = 0.0;
	Sampled = 0;
	GpuZero = 0;
	Result = FBenchResult();

	State = WarmupFrames > 0 ? EState::Warmup : EState::Sampling;
	FramesLeft = WarmupFrames > 0 ? WarmupFrames : SampleFrames;

	// 프레임 캡이 걸려 있으면 frame_ms가 실제 부하가 아니라 캡 값에 붙는다.
	const int32 MaxFps = GetIntCVar(TEXT("t.MaxFPS"));
	const int32 VSync = GetIntCVar(TEXT("r.VSync"));
	const bool bSmooth = GEngine && GEngine->bSmoothFrameRate;
	if (MaxFps > 0 || VSync > 0 || bSmooth)
	{
		UE_LOG(LogBench, Warning,
			TEXT("[Bench] frame cap active (t.MaxFPS=%d r.VSync=%d smooth=%d). frame_ms may be capped; set t.MaxFPS 0, r.VSync 0"),
			MaxFps, VSync, bSmooth ? 1 : 0);
	}

	UE_LOG(LogBench, Display, TEXT("[Bench] record start: %s x%d moving=%d alive=%d (warmup=%d, samples=%d)"),
		Bench::ModeToString(Info.Mode), Info.Count, Info.bMoving ? 1 : 0, Info.Alive, WarmupFrames, SampleFrames);
}

void FBenchRecorder::Cancel()
{
	if (State != EState::Idle)
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] record cancelled (%d/%d samples)"), Sampled, SampleFrames);
	}
	State = EState::Idle;
}

bool FBenchRecorder::Tick(float DeltaSeconds)
{
	if (State == EState::Idle)
	{
		return false;
	}

	if (State == EState::Warmup)
	{
		if (--FramesLeft <= 0)
		{
			State = EState::Sampling;
			FramesLeft = SampleFrames;
			TRACE_BOOKMARK(TEXT("Bench Sample Start: %s x%d moving=%d"), Bench::ModeToString(Info.Mode), Info.Count, Info.bMoving ? 1 : 0);
		}
		return false;
	}

	// Sampling: 직전 프레임에 엔진이 갱신한 값을 읽는다.
	const double FrameMs  = static_cast<double>(DeltaSeconds) * 1000.0;
	const double GameMs   = FPlatformTime::ToMilliseconds(GGameThreadTime);
	const double RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
	const double RhiMs    = FPlatformTime::ToMilliseconds(GRHIThreadTime);
	const uint32 GpuCycles = RHIGetGPUFrameCycles(0);
	const double GpuMs    = FPlatformTime::ToMilliseconds(GpuCycles);

	SumFrameMs  += FrameMs;
	SumGameMs   += GameMs;
	SumRenderMs += RenderMs;
	SumRhiMs    += RhiMs;
	SumGpuMs    += GpuMs;
	MaxFrameMs   = FMath::Max(MaxFrameMs, FrameMs);
	if (GpuCycles == 0)
	{
		++GpuZero;
	}
	++Sampled;

	if (--FramesLeft <= 0)
	{
		Finish();
		return true;
	}
	return false;
}

void FBenchRecorder::Finish()
{
	const double N = FMath::Max(1, Sampled);

	Result.Frames      = Sampled;
	Result.FrameMs     = SumFrameMs / N;
	Result.FrameMaxMs  = MaxFrameMs;
	Result.GameMs      = SumGameMs / N;
	Result.RenderMs    = SumRenderMs / N;
	Result.RhiMs       = SumRhiMs / N;
	Result.GpuMs       = SumGpuMs / N;
	Result.GpuZeroFrames = GpuZero;

	State = EState::Idle;

	TRACE_BOOKMARK(TEXT("Bench Sample End: %s x%d moving=%d"), Bench::ModeToString(Info.Mode), Info.Count, Info.bMoving ? 1 : 0);

	WriteCsvRow();

	UE_LOG(LogBench, Display,
		TEXT("[Bench] result: %s x%d moving=%d | frame %.2f (max %.2f) game %.2f render %.2f rhi %.2f gpu %.2f ms | n=%d"),
		Bench::ModeToString(Info.Mode), Info.Count, Info.bMoving ? 1 : 0,
		Result.FrameMs, Result.FrameMaxMs, Result.GameMs, Result.RenderMs, Result.RhiMs, Result.GpuMs, Result.Frames);

	if (GpuZero > 0)
	{
		UE_LOG(LogBench, Warning, TEXT("[Bench] GPU time was 0 in %d/%d frames - GPU timing may be unavailable"), GpuZero, Sampled);
	}
}

void FBenchRecorder::WriteCsvRow() const
{
	const FString Path = GetCsvPath();
	const bool bNewFile = !IFileManager::Get().FileExists(*Path);

	FString Out;
	if (bNewFile)
	{
		Out += TEXT("timestamp,mode_id,mode,count,alive,moving,warmup,frames,")
		       TEXT("frame_ms,frame_max_ms,game_ms,render_ms,rhi_ms,gpu_ms,gpu_zero,")
		       TEXT("res_x,res_y,max_fps,vsync,smooth,gpu_name,engine,build,tag\n");
	}

	const FIntPoint Res = GetViewportSize();
	const int32 MaxFps = GetIntCVar(TEXT("t.MaxFPS"));
	const int32 VSync = GetIntCVar(TEXT("r.VSync"));
	const bool bSmooth = GEngine && GEngine->bSmoothFrameRate;

#if WITH_EDITOR
	const TCHAR* Build = GIsEditor ? TEXT("Editor") : TEXT("EditorGame");
#elif UE_BUILD_SHIPPING
	const TCHAR* Build = TEXT("Shipping");
#else
	const TCHAR* Build = TEXT("Development");
#endif

	Out += FString::Printf(
		TEXT("%s,%d,%s,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%s,%s,%s,%s\n"),
		*FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")),
		static_cast<int32>(Info.Mode), Bench::ModeToString(Info.Mode),
		Info.Count, Info.Alive, Info.bMoving ? 1 : 0,
		WarmupFrames, Result.Frames,
		Result.FrameMs, Result.FrameMaxMs, Result.GameMs, Result.RenderMs, Result.RhiMs, Result.GpuMs, Result.GpuZeroFrames,
		Res.X, Res.Y, MaxFps, VSync, bSmooth ? 1 : 0,
		*CsvQuote(GRHIAdapterName),
		*CsvQuote(FEngineVersion::Current().ToString()),
		Build,
		*Info.Tag);

	FFileHelper::SaveStringToFile(Out, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);

	UE_LOG(LogBench, Display, TEXT("[Bench] csv -> %s"), *FPaths::ConvertRelativePathToFull(Path));
}
