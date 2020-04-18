// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "TrueSkySequenceAsset.generated.h"

UCLASS()
class TRUESKYPLUGIN_API UTrueSkySequenceAsset : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DELEGATE_OneParam(Destroyed, UTrueSkySequenceAsset*const);
	Destroyed OnDestroyed;

	UPROPERTY()
	TArray<uint8> SequenceText;
	
	UPROPERTY()
	FString Description;

	void BeginDestroy();
};
