#include "Spawners/BenchISMSpawner.h"
#include "BenchLayout.h"
#include "BenchTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

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

	BaseLocations.SetNumUninitialized(Count);
	TArray<FTransform> Transforms;
	Transforms.SetNumUninitialized(Count);
	const FVector Scale(Bench::Layout::MeshScale);
	for (int32 i = 0; i < Count; ++i)
	{
		BaseLocations[i] = Bench::Layout::GetBaseLocation(i, Count);
		Transforms[i] = FTransform(FQuat::Identity, BaseLocations[i], Scale);
	}

	const bool bShader = (Update == EISMUpdate::ShaderMove);
	ISM->SetNumCustomDataFloats(bShader ? 1 : 0);

	ISM->PreAllocateInstancesMemory(Count);
	ISM->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);

	if (bShader)
	{
		// 인스턴스별 위상만 1회 기록. CPU가 매 프레임 할 일은 이것으로 끝.
		// 위상은 2π로 감아 float 정밀도 손실을 막는다 (5만 x 0.37 = 18500 rad 그대로 넣으면 sin 정밀도가 떨어짐).
		for (int32 i = 0; i < Count; ++i)
		{
			const float Phase = static_cast<float>(FMath::Fmod(Bench::Layout::MovePhaseStep * i, UE_TWO_PI));
			ISM->SetCustomDataValue(i, 0, Phase, /*bMarkRenderStateDirty*/ i == Count - 1);
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
	BaseLocations.Reset();
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

	const int32 Num = BaseLocations.Num();
	const FVector Scale(Bench::Layout::MeshScale);
	for (int32 i = 0; i < Num; ++i)
	{
		const FTransform T(FQuat::Identity, BaseLocations[i] + Bench::Layout::GetMoveOffset(i, TimeSeconds), Scale);
		const bool bMarkDirty = (Update == EISMUpdate::DirtyOnce) ? (i == Num - 1) : true;
		ISM->UpdateInstanceTransform(i, T, /*bWorldSpace*/ true, bMarkDirty, /*bTeleport*/ true);
	}
}
