// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "GameFramework/Info.h"
#include "TrueSkyLight.generated.h"

UCLASS(ClassGroup=Lights,hidecategories=(Input,Collision,Replication,Info), showcategories=("Rendering", "Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, ConversionRoot, Blueprintable)
class TRUESKYPLUGIN_API ATrueSkyLight : public AInfo
{
	GENERATED_UCLASS_BODY()

private:
	/** The main component */
	UPROPERTY(Category = TrueSky,VisibleAnywhere, BlueprintReadOnly,meta = (ExposeFunctionCategories = "Light,Rendering,TrueSky,Rendering|Components|SkyLight", AllowPrivateAccess = "true"))
	class UTrueSkyLightComponent* LightComponent;

public:
	/** replicated copy of LightComponent's bEnabled property */
	UPROPERTY(replicatedUsing=OnRep_bEnabled)
	uint32 bEnabled:1;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_bEnabled();

	/** Returns LightComponent subobject **/
	class UTrueSkyLightComponent* GetLightComponent() const;
};



