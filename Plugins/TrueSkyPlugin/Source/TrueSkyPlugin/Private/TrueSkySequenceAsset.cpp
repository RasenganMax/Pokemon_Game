// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#include "TrueSkySequenceAsset.h"

#include "TrueSkyPluginPrivatePCH.h"

UTrueSkySequenceAsset::UTrueSkySequenceAsset(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{

}

void UTrueSkySequenceAsset::BeginDestroy()
{
	OnDestroyed.ExecuteIfBound(this);

	Super::BeginDestroy();
}
