#include "Spawners/BenchMeshActor.h"
#include "BenchLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

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

void ABenchMeshActor::Setup(UStaticMesh* InMesh, int32 InIndex, const FVector& InBaseLocation)
{
	Index = InIndex;
	BaseLocation = InBaseLocation;
	Mesh->SetStaticMesh(InMesh);
	Mesh->SetWorldScale3D(FVector(Bench::Layout::MeshScale));
}

void ABenchMeshActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double T = GetWorld()->GetTimeSeconds();
	SetActorLocation(BaseLocation + Bench::Layout::GetMoveOffset(Index, T), false, nullptr, ETeleportType::TeleportPhysics);
}
