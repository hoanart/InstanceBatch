#pragma once

#include "CoreMinimal.h"
#include "BenchTypes.generated.h"

BENCH_API DECLARE_LOG_CATEGORY_EXTERN(LogBench, Log, All);

/** lab.Mode 값과 1:1 대응. 숫자를 바꾸면 CSV의 mode 컬럼 의미도 바뀌므로 순서 고정. */
UENUM()
enum class EBenchMode : uint8
{
	Actor          = 0,
	ISM_PerInstance = 1,	// 인스턴스마다 UpdateInstanceTransform + 더티 마킹
	ISM_Batched    = 2,	// 전부 갱신 후 MarkRenderStateDirty 1회
	ISM_ShaderMove = 3,	// PerInstanceCustomData + 머티리얼에서 이동 계산
	HISM           = 4,
	Mass           = 5,

	Count UMETA(Hidden)
};

namespace Bench
{
	BENCH_API const TCHAR* ModeToString(EBenchMode Mode);
}
