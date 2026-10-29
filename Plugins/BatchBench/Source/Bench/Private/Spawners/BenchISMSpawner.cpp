#include "Spawners/BenchISMSpawner.h"
#include "BenchLayout.h"
#include "BenchTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	const TCHAR* UpdateName(EISMUpdate U)
	{
		switch (U)
		{
		case EISMUpdate::DirtyEach:  return TEXT("DirtyEach");
		case EISMUpdate::DirtyOnce:  return TEXT("DirtyOnce");
		case EISMUpdate::ShaderMove: return TEXT("ShaderMove");
		}
		return TEXT("?");
	}
}

void UBenchISMSpawner::Setup(UWorld* InWorld, UStaticMesh* InMesh, EISMUpdate InUpdate, UMaterialInterface* InMoveMaterial)
{
	World = InWorld;
	Mesh = InMesh;
	Update = InUpdate;
	MoveMaterial = InMoveMaterial;
}

void UBenchISMSpawner::SetMoving(bool bInMoving)
{
	bMoving = bInMoving;

	if (Update == EISMUpdate::ShaderMove && IsValid(ISM))
	{
		// 이동 = WPO 머티리얼로 교체, 정지 = 메시 기본 머티리얼(nullptr 오버라이드 해제).
		// 교체는 1회 일어나는 일이라 측정 구간(워밍업 이후)에는 영향이 없다.
		ISM->SetMaterial(0, bMoving ? MoveMaterial.Get() : nullptr);
	}
}

void UBenchISMSpawner::EnsureComponent()
{
	if (IsValid(ISM))
	{
		return;
	}
	UWorld* W = World.Get();
	if (!W)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Host = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);

	ISM = NewObject<UInstancedStaticMeshComponent>(Host, TEXT("BenchISM"));
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// Actor 방식과 조건 맞춤
	ISM->SetGenerateOverlapEvents(false);
	ISM->SetCanEverAffectNavigation(false);
	ISM->SetStaticMesh(Mesh);
	Host->SetRootComponent(ISM);
	ISM->RegisterComponent();
}

void UBenchISMSpawner::Spawn(int32 Count)
{
	Clear();
	EnsureComponent();
	if (!ISM)
	{
		return;
	}

	const double StartTime = FPlatformTime::Seconds();

	SpawnedCount = Count;
	Rotations.SetNumUninitialized(Count);
	Dirs.SetNumUninitialized(Count);
	R0s.SetNumUninitialized(Count);
	TArray<FTransform> Transforms;
	Transforms.SetNumUninitialized(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		Dirs[i] = Bench::Layout::GetDirection(i);
		R0s[i] = Bench::Layout::GetInitialRadius(i, Count);
		Rotations[i] = Dirs[i].Rotation().Quaternion();
		Transforms[i] = FTransform(Rotations[i], Bench::Layout::GetLocationCached(Dirs[i], R0s[i], 0.0), Bench::Layout::MeshScale);
	}

	// ShaderMove: CustomData 0..2 = 비행 방향, 3 = 초기 반경. 머티리얼이 같은 식으로 위치를 계산한다.
	const bool bShader = (Update == EISMUpdate::ShaderMove);
	ISM->SetNumCustomDataFloats(bShader ? 4 : 0);

	ISM->PreAllocateInstancesMemory(Count);
	ISM->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);

	if (bShader)
	{
		// 인스턴스별 방향·초기 반경을 1회 기록. CPU가 매 프레임 할 일은 이것으로 끝.
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector& Dir = Dirs[i];
			const float R0 = static_cast<float>(R0s[i]);
			const bool bLast = (i == Count - 1);
			ISM->SetCustomDataValue(i, 0, static_cast<float>(Dir.X), false);
			ISM->SetCustomDataValue(i, 1, static_cast<float>(Dir.Y), false);
			ISM->SetCustomDataValue(i, 2, static_cast<float>(Dir.Z), false);
			ISM->SetCustomDataValue(i, 3, R0, bLast);
		}
	}

	UE_LOG(LogBench, Display, TEXT("[Bench] ISM(%s) spawn x%d took %.1f ms"),
		UpdateName(Update), ISM->GetInstanceCount(), (FPlatformTime::Seconds() - StartTime) * 1000.0);
}

void UBenchISMSpawner::Clear()
{
	if (IsValid(ISM))
	{
		ISM->ClearInstances();
	}
	Rotations.Reset();
	Dirs.Reset();
	R0s.Reset();
	SpawnedCount = 0;
}

int32 UBenchISMSpawner::GetCount() const
{
	return IsValid(ISM) ? ISM->GetInstanceCount() : 0;
}

void UBenchISMSpawner::TickSpawner(float DeltaTime, double TimeSeconds)
{
	// ShaderMove는 CPU 갱신이 없다. 이 함수가 아무 일도 안 하는 것 자체가 비교 포인트.
	if (!bMoving || !IsValid(ISM) || Update == EISMUpdate::ShaderMove)
	{
		return;
	}

	// Insights에서 "이동 갱신 비용이 어디서 나오는가"를 보기 위한 스코프.
	// DirtyEach / DirtyOnce 를 같은 이름으로 두면 비교가 쉽다.
	TRACE_CPUPROFILER_EVENT_SCOPE(Bench_ISM_UpdateInstances);

	const int32 Num = Rotations.Num();
	for (int32 i = 0; i < Num; ++i)
	{
		const FTransform T(Rotations[i], Bench::Layout::GetLocationCached(Dirs[i], R0s[i], TimeSeconds), Bench::Layout::MeshScale);
		const bool bMarkDirty = (Update == EISMUpdate::DirtyOnce) ? (i == Num - 1) : true;
		ISM->UpdateInstanceTransform(i, T, /*bWorldSpace*/ true, bMarkDirty, /*bTeleport*/ true);
	}
}
