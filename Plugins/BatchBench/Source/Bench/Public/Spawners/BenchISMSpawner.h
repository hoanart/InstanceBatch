#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IBatchSpawner.h"
#include "BenchISMSpawner.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UWorld;
class AActor;

class UMaterialInterface;

/** ISM 이동 처리 방식. */
enum class EISMUpdate : uint8
{
	DirtyEach,	// UpdateInstanceTransform(..., bMarkRenderStateDirty=true) 를 모든 인스턴스에
	DirtyOnce,	// 같은 갱신, 더티 플래그는 마지막 인스턴스에만 (엔진 헤더 주석 권장 방식)
	ShaderMove,	// CPU 갱신 없음. 인스턴스별 위상만 CustomData로 1회 넣고, 이동은 머티리얼 WPO가 계산
};

/**
 * 방식 1/2/3: InstancedStaticMeshComponent 1개에 전부 담는다.
 *
 * DirtyEach 와 DirtyOnce 는 이동 갱신에서 더티 플래그 횟수 하나만 다르다.
 * ShaderMove 는 "갱신 비용을 줄이는 것"이 아니라 "CPU 갱신 자체를 없애는 것"의 비교 대상.
 * 배치·메시·충돌·그림자는 모두 동일하게 두어 변수를 통제한다.
 */
UCLASS()
class BENCH_API UBenchISMSpawner : public UObject, public IBatchSpawner
{
	GENERATED_BODY()

public:
	void Setup(UWorld* InWorld, UStaticMesh* InMesh, EISMUpdate InUpdate, UMaterialInterface* InMoveMaterial = nullptr);

	// IBatchSpawner
	virtual void Spawn(int32 Count) override;
	virtual void SetMoving(bool bInMoving) override;
	virtual void Clear() override;
	virtual int32 GetCount() const override;
	virtual void TickSpawner(float DeltaTime, double TimeSeconds) override;

private:
	void EnsureComponent();

	TWeakObjectPtr<UWorld> World;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY()
	TObjectPtr<AActor> Host;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ISM;

	/** 스폰 시 고정되는 인스턴스별 회전. 위치만 매 프레임 바뀐다. */
	TArray<FQuat> Rotations;
	/** 스폰 시 계산해 두는 방향·초기 반경. 매 프레임 루프에서 다시 계산하지 않는다. */
	TArray<FVector> Dirs;
	TArray<double> R0s;
	int32 SpawnedCount = 0;
	EISMUpdate Update = EISMUpdate::DirtyEach;

	/** ShaderMove 전용: 이동 시 덮어쓸 머티리얼 (WPO로 이동). 정지 시엔 메시 기본 머티리얼. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> MoveMaterial;
	bool bMoving = false;
};
