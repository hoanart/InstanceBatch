#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IBatchSpawner.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UBatchSpawner : public UInterface
{
	GENERATED_BODY()
};

/**
 * 모든 방식(Actor/ISM/HISM/Mass)이 따르는 공통 인터페이스.
 * 비교 조건을 같게 강제하는 게 목적이므로, 방식별 최적화 옵션은 여기 넣지 않는다.
 */
class BENCH_API IBatchSpawner
{
	GENERATED_BODY()

public:
	/** 기존 객체를 지우고 Count개를 새로 만든다. */
	virtual void Spawn(int32 Count) = 0;

	/** true면 매 프레임 이동, false면 정지. */
	virtual void SetMoving(bool bMoving) = 0;

	/** 만든 객체를 전부 제거한다. */
	virtual void Clear() = 0;

	/** 현재 살아 있는 객체 수 (측정 기록 검증용). */
	virtual int32 GetCount() const = 0;
};
