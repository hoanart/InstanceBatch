#pragma once

#include "CoreMinimal.h"

/**
 * 모든 방식이 똑같은 위치·크기·움직임을 쓰도록 배치 규칙을 한 곳에 둔다.
 * 방식마다 배치가 다르면 화면 점유율·오버드로우가 달라져 비교가 무너진다.
 *
 * 장면: 발사 지점 하나에서 부채꼴로 퍼지는 탄막.
 *   탄환 i 는 고유한 방향 Dir(i)와 초기 반경 R0(i)를 갖고,
 *   시간 t 에서 발사점으로부터  R(i,t) = fmod(R0(i) + Speed * t, Range)  만큼 떨어진 곳에 있다.
 *   사거리 끝에 닿으면 발사점으로 돌아가므로 계속 쏘는 모양이 된다.
 * ShaderMove 머티리얼(M_Bench_ShaderMove)은 같은 식을 CustomData(dir xyz, r0)로 계산한다.
 */
namespace Bench::Layout
{
	/** 발사 지점 (월드, cm). */
	constexpr double OriginX = 0.0;
	constexpr double OriginY = 0.0;
	constexpr double OriginZ = 300.0;

	/** 사거리(cm)와 탄속(cm/s). 5만 발이 사거리 안에 고르게 분포한다. */
	constexpr double Range = 6000.0;
	constexpr double Speed = 1500.0;

	/** 부채꼴 각도 (도). 수평 ±Yaw, 수직 -PitchDown ~ +PitchUp. */
	constexpr double HalfYawDeg = 60.0;
	constexpr double PitchUpDeg = 25.0;
	constexpr double PitchDownDeg = -5.0;

	/** 탄환 메시 스케일. Cube(100cm) 기준 X 2.0 = 200cm 길이, 단면 15cm. 정점 수는 그대로 12. */
	inline const FVector MeshScale(2.0, 0.15, 0.15);

	/** 탄환 i의 비행 방향 (단위 벡터). 인덱스 기반 결정적 분포라 모든 방식·실행에서 동일. */
	BENCH_API FVector GetDirection(int32 Index);

	/** 탄환 i의 초기 반경. 사거리 안에 고르게 흩어진다. */
	BENCH_API double GetInitialRadius(int32 Index, int32 Count);

	/** 시간 t 에서 탄환 i 의 발사점으로부터의 거리. */
	FORCEINLINE double GetRadius(int32 Index, int32 Count, double TimeSeconds)
	{
		return FMath::Fmod(GetInitialRadius(Index, Count) + Speed * TimeSeconds, Range);
	}

	/** 시간 t 에서 탄환 i 의 월드 위치. (방향·반경을 매번 계산하므로 스폰 시점용) */
	BENCH_API FVector GetLocation(int32 Index, int32 Count, double TimeSeconds);

	/** 매 프레임용. 방향·초기 반경은 스폰 때 계산해 둔 값을 넘긴다. 루프에 남는 건 fmod 1회 + 벡터 곱셈뿐. */
	FORCEINLINE FVector GetLocationCached(const FVector& Dir, double R0, double TimeSeconds)
	{
		return FVector(OriginX, OriginY, OriginZ) + Dir * FMath::Fmod(R0 + Speed * TimeSeconds, Range);
	}

	/** 탄환 i 의 회전 (진행 방향을 +X로). */
	BENCH_API FRotator GetRotation(int32 Index);

	/** 정지 상태 기준 위치 = t=0. */
	FORCEINLINE FVector GetBaseLocation(int32 Index, int32 Count)
	{
		return GetLocation(Index, Count, 0.0);
	}

	/** 고정 카메라. 옆에서 부채꼴 전체가 보이는 위치. */
	BENCH_API FVector GetCameraLocation();
	BENCH_API FRotator GetCameraRotation();
}
