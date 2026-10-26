#pragma once

#include "HAL/IConsoleManager.h"

/** lab.* CVar. 정의는 BenchCVars.cpp. 모듈 내부 전용. */
namespace BenchCVars
{
	extern TAutoConsoleVariable<int32> Mode;
	extern TAutoConsoleVariable<int32> Count;
	extern TAutoConsoleVariable<int32> Moving;
	extern TAutoConsoleVariable<int32> Record;
	extern TAutoConsoleVariable<int32> WarmupFrames;
	extern TAutoConsoleVariable<int32> SampleFrames;
	extern TAutoConsoleVariable<int32> Sweep;
	extern TAutoConsoleVariable<FString> SweepModes;
	extern TAutoConsoleVariable<FString> SweepCounts;
}
