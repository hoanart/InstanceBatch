#pragma once

#include "CoreMinimal.h"

/**
 * 모든 방식이 똑같은 위치·크기·움직임을 쓰도록 배치 규칙을 한 곳에 둔다.
 * 방식마다 배치가 다르면 화면 점유율·오버드로우가 달라져 비교가 무너진다.
 */
namespace Bench::Layout
{
	/** 격자 간격(cm)과 메시 스케일. Cube(100cm) * 0.3 = 30cm 큐브, 간격 60cm. */
	constexpr double Spacing = 60.0;
	constexpr double MeshScale = 0.3;

	/** 격자 바닥 높이(cm). */
	constexpr double BaseZ = 100.0;

	/** 이동: Z축 사인 진동. 진폭(cm), 각속도(rad/s), 인덱스별 위상차. */
	constexpr double MoveAmplitude = 50.0;
	constexpr double MoveSpeed = 2.0;
	constexpr double MovePhaseStep = 0.37;

	/** Count개를 정육면체에 가까운 3D 격자로 배치한 i번째 기준 위치 (월드). */
	BENCH_API FVector GetBaseLocation(int32 Index, int32 Count);

	/** 시간 t에서의 이동 오프셋. */
	FORCEINLINE FVector GetMoveOffset(int32 Index, double TimeSeconds)
	{
		return FVector(0.0, 0.0, MoveAmplitude * FMath::Sin(MoveSpeed * TimeSeconds + MovePhaseStep * Index));
	}

	/** 고정 카메라 위치/회전. 50k 격자(약 22m 정육면체)가 다 들어오도록 잡았다. */
	BENCH_API FVector GetCameraLocation();
	BENCH_API FRotator GetCameraRotation();
}
