#include "Spawners/BenchMeshActor.h"
#include "BenchLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

ABenchMeshActor::ABenchMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;	// 정지 상태에서는 Tick 비용 0 (SetMoving에서 켠다)

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// ISM과 조건 맞춤: 충돌 없음
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	RootComponent = Mesh;
}

void ABenchMeshActor::Setup(UStaticMesh* InMesh, int32 InIndex, int32 InCount)
{
	Index = InIndex;
	Dir = Bench::Layout::GetDirection(InIndex);
	R0 = Bench::Layout::GetInitialRadius(InIndex, InCount);
	Mesh->SetStaticMesh(InMesh);
	Mesh->SetWorldScale3D(Bench::Layout::MeshScale);
}

void ABenchMeshActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TRACE_CPUPROFILER_EVENT_SCOPE(Bench_Actor_Tick);
	const double T = GetWorld()->GetTimeSeconds();
	SetActorLocation(Bench::Layout::GetLocationCached(Dir, R0, T), false, nullptr, ETeleportType::TeleportPhysics);
}
