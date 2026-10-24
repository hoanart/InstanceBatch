#include "BenchTypes.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogBench);

namespace Bench
{
	const TCHAR* ModeToString(EBenchMode Mode)
	{
		switch (Mode)
		{
		case EBenchMode::Actor:           return TEXT("Actor");
		case EBenchMode::ISM_PerInstance: return TEXT("ISM-PerInstance");
		case EBenchMode::ISM_Batched:     return TEXT("ISM-Batched");
		case EBenchMode::ISM_ShaderMove:  return TEXT("ISM-ShaderMove");
		case EBenchMode::HISM:            return TEXT("HISM");
		case EBenchMode::Mass:            return TEXT("Mass");
		default:                          return TEXT("Unknown");
		}
	}
}

class FBenchModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogBench, Display, TEXT("[Bench] module loaded"));
	}
};

IMPLEMENT_MODULE(FBenchModule, Bench)
