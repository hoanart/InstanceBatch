#pragma once

#include "CoreMinimal.h"
#include "BenchTypes.generated.h"

BENCH_API DECLARE_LOG_CATEGORY_EXTERN(LogBench, Log, All);

/** lab.Mode 값과 1:1 대응. 숫자를 바꾸면 CSV의 mode 컬럼 의미도 바뀌므로 순서 고정. */
UENUM()
enum class EBenchMode : uint8
{
	Actor          = 0,
	ISM_DirtyEach  = 1,	// 트랜스폼 갱신 + 더티 플래그를 인스턴스마다
	ISM_DirtyOnce  = 2,	// 트랜스폼 갱신 + 더티 플래그는 마지막 1회만
	ISM_ShaderMove = 3,	// CPU 갱신 없음. PerInstanceCustomData(위상) + 머티리얼 WPO로 이동
	HISM           = 4,
	Mass           = 5,

	Count UMETA(Hidden)
};

namespace Bench
{
	BENCH_API const TCHAR* ModeToString(EBenchMode Mode);
}
