#include "BenchLayout.h"

namespace Bench::Layout
{
	FVector GetBaseLocation(int32 Index, int32 Count)
	{
		const int32 Side = FMath::Max(1, FMath::CeilToInt(FMath::Pow(static_cast<double>(FMath::Max(1, Count)), 1.0 / 3.0)));
		const int32 X = Index % Side;
		const int32 Y = (Index / Side) % Side;
		const int32 Z = Index / (Side * Side);

		// 격자 중심을 원점(XY)에 맞춘다.
		const double Half = (Side - 1) * Spacing * 0.5;
		return FVector(X * Spacing - Half, Y * Spacing - Half, BaseZ + Z * Spacing);
	}

	FVector GetCameraLocation()
	{
		return FVector(-4200.0, 0.0, 1400.0);
	}

	FRotator GetCameraRotation()
	{
		// 격자 중심 근처(0,0,1200)를 약간 내려다보는 각도.
		return FRotator(-3.0, 0.0, 0.0);
	}
}
