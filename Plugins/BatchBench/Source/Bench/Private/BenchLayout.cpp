#include "BenchLayout.h"

namespace Bench::Layout
{
	namespace
	{
		/** 0..1 결정적 해시. 인덱스가 같으면 항상 같은 값. */
		FORCEINLINE double Hash01(uint32 X)
		{
			X ^= X >> 16; X *= 0x7feb352dU;
			X ^= X >> 15; X *= 0x846ca68bU;
			X ^= X >> 16;
			return static_cast<double>(X & 0x00FFFFFFU) / static_cast<double>(0x01000000U);
		}
	}

	FVector GetDirection(int32 Index)
	{
		const double YawDeg   = FMath::Lerp(-HalfYawDeg, HalfYawDeg, Hash01(static_cast<uint32>(Index) * 2u + 1u));
		const double PitchDeg = FMath::Lerp(PitchDownDeg, PitchUpDeg, Hash01(static_cast<uint32>(Index) * 2u + 2u));
		return FRotator(PitchDeg, YawDeg, 0.0).Vector();
	}

	double GetInitialRadius(int32 Index, int32 Count)
	{
		// 인덱스 순으로 사거리를 균등 분할. 같은 Count면 분포가 항상 같다.
		const int32 N = FMath::Max(1, Count);
		return (static_cast<double>(Index % N) + 0.5) / static_cast<double>(N) * Range;
	}

	FVector GetLocation(int32 Index, int32 Count, double TimeSeconds)
	{
		return FVector(OriginX, OriginY, OriginZ) + GetDirection(Index) * GetRadius(Index, Count, TimeSeconds);
	}

	FRotator GetRotation(int32 Index)
	{
		return GetDirection(Index).Rotation();
	}

	FVector GetCameraLocation()
	{
		// 발사점의 오른쪽 뒤에서 부채꼴을 비스듬히 내려다본다.
		return FVector(-2200.0, 5200.0, 2300.0);
	}

	FRotator GetCameraRotation()
	{
		return FRotator(-14.0, -52.0, 0.0);
	}
}
