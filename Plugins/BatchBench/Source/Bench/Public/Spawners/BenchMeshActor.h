#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchMeshActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

/** Actor 방식의 객체 1개. 이동은 자기 Tick에서 처리한다 (일반적인 Actor 사용 패턴). */
UCLASS(NotBlueprintable, Transient)
class BENCH_API ABenchMeshActor : public AActor
{
	GENERATED_BODY()

public:
	ABenchMeshActor();

	void Setup(UStaticMesh* Mesh, int32 InIndex, int32 InCount);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	int32 Index = 0;
	FVector Dir = FVector::ForwardVector;
	double R0 = 0.0;
};
