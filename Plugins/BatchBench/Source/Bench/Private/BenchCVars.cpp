#include "BenchCVars.h"

namespace BenchCVars
{
	TAutoConsoleVariable<int32> Mode(
		TEXT("lab.Mode"),
		0,
		TEXT("벤치 방식 선택.\n")
		TEXT(" 0=Actor 1=ISM-DirtyEach 2=ISM-DirtyOnce 3=ISM-ShaderMove 4=HISM 5=Mass"),
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
		TEXT("1=측정 시작 (워밍업 후 평균을 Saved/Bench/bench_results.csv에 기록, 끝나면 자동으로 0). 0=측정 취소"),
		ECVF_Default);

	TAutoConsoleVariable<int32> WarmupFrames(
		TEXT("lab.WarmupFrames"),
		60,
		TEXT("측정 전에 버릴 프레임 수 (스폰 직후 스파이크·셰이더 컴파일 제외용)"),
		ECVF_Default);

	TAutoConsoleVariable<int32> SampleFrames(
		TEXT("lab.SampleFrames"),
		60,
		TEXT("평균에 쓸 프레임 수"),
		ECVF_Default);

	TAutoConsoleVariable<int32> Sweep(
		TEXT("lab.Sweep"),
		0,
		TEXT("1=자동 측정 시작 (lab.Sweep.Modes x lab.Sweep.Counts x 정지/이동 전 조합). 0=중단"),
		ECVF_Default);

	TAutoConsoleVariable<FString> SweepModes(
		TEXT("lab.Sweep.Modes"),
		TEXT("0,1,2,3"),
		TEXT("자동 측정할 모드 목록 (쉼표 구분). 정지 상태의 1,2,3은 같아야 정상 -> 측정 노이즈 확인용"),
		ECVF_Default);

	TAutoConsoleVariable<FString> SweepCounts(
		TEXT("lab.Sweep.Counts"),
		TEXT("1000,5000,20000,50000"),
		TEXT("자동 측정할 객체 수 목록 (쉼표 구분)"),
		ECVF_Default);
}
