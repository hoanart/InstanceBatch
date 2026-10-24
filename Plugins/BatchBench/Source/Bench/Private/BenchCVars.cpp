#include "BenchCVars.h"

namespace BenchCVars
{
	TAutoConsoleVariable<int32> Mode(
		TEXT("lab.Mode"),
		0,
		TEXT("벤치 방식 선택.\n")
		TEXT(" 0=Actor 1=ISM-PerInstance 2=ISM-Batched 3=ISM-ShaderMove 4=HISM 5=Mass"),
		ECVF_Default);

	TAutoConsoleVariable<int32> Count(
		TEXT("lab.Count"),
		1000,
		TEXT("스폰할 객체 수. 권장 단계: 1000 / 5000 / 20000 / 50000"),
		ECVF_Default);

	TAutoConsoleVariable<int32> Moving(
		TEXT("lab.Moving"),
		0,
		TEXT("0=정지, 1=매 프레임 이동"),
		ECVF_Default);

	TAutoConsoleVariable<int32> Record(
		TEXT("lab.Record"),
		0,
		TEXT("1로 설정하면 측정 시작 (워밍업 후 평균을 CSV에 기록). 2단계에서 구현."),
		ECVF_Default);
}
