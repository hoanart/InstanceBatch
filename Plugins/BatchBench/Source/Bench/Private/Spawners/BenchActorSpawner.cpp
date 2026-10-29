#include "Spawners/BenchActorSpawner.h"
#include "Spawners/BenchMeshActor.h"
#include "BenchLayout.h"
#include "BenchTypes.h"
#include "Engine/World.h"

void UBenchActorSpawner::Setup(UWorld* InWorld, UStaticMesh* InMesh)
{
	World = InWorld;
	Mesh = InMesh;
}

void UBenchActorSpawner::Spawn(int32 Count)
{
	Clear();
	UWorld* W = World.Get();
	if (!W || !Mesh)
	{
		return;
	}

	const double StartTime = FPlatformTime::Seconds();
	Actors.Reserve(Count);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Loc = Bench::Layout::GetBaseLocation(i, Count);
		ABenchMeshActor* A = W->SpawnActor<ABenchMeshActor>(Loc, Bench::Layout::GetRotation(i), Params);
		if (A)
		{
			A->Setup(Mesh, i, Count);
			A->SetActorTickEnabled(bMoving);
			Actors.Add(A);
		}
	}

	UE_LOG(LogBench, Display, TEXT("[Bench] Actor spawn x%d took %.1f ms"), Actors.Num(), (FPlatformTime::Seconds() - StartTime) * 1000.0);
}

void UBenchActorSpawner::SetMoving(bool bInMoving)
{
	bMoving = bInMoving;
	for (ABenchMeshActor* A : Actors)
	{
		if (A)
		{
			A->SetActorTickEnabled(bMoving);
		}
	}
}

void UBenchActorSpawner::Clear()
{
	for (ABenchMeshActor* A : Actors)
	{
		if (IsValid(A))
		{
			A->Destroy();
		}
	}
	Actors.Reset();
}
