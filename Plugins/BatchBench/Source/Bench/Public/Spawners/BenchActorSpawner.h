#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IBatchSpawner.h"
#include "BenchActorSpawner.generated.h"

class ABenchMeshActor;
class UStaticMesh;
class UWorld;

/** 방식 0: 객체마다 Actor 1개 + StaticMeshComponent 1개. */
UCLASS()
class BENCH_API UBenchActorSpawner : public UObject, public IBatchSpawner
{
	GENERATED_BODY()

public:
	void Setup(UWorld* InWorld, UStaticMesh* InMesh);

	// IBatchSpawner
	virtual void Spawn(int32 Count) override;
	virtual void SetMoving(bool bMoving) override;
	virtual void Clear() override;
	virtual int32 GetCount() const override { return Actors.Num(); }

private:
	TWeakObjectPtr<UWorld> World;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY()
	TArray<TObjectPtr<ABenchMeshActor>> Actors;

	bool bMoving = false;
};
