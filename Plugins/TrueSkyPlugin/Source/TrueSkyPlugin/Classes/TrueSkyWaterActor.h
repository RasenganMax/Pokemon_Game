// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Misc/Variant.h"
#include "UObject/ObjectMacros.h"

#include "Engine/Texture2D.h"

#include "GameFramework/Volume.h"
#include "Components/BrushComponent.h"

#include "TrueSkyPropertiesFloat.h"
#include "TrueSkyPropertiesInt.h"

#include "ITrueSkyPlugin.h"
#include "TrueSkyWaterActor.generated.h"

UENUM()
enum class EWaterColour : uint8
{
	Clear = 0x0,
	Ocean = 0x1,
	Muddy = 0x2
};

UCLASS(ClassGroup = Water, hidecategories = (Collision, Replication, Info), showcategories = ("Rendering"), NotBlueprintable)
class TRUESKYPLUGIN_API ATrueSkyWater : public AVolume
{
	GENERATED_BODY()
		
protected:
	~ATrueSkyWater();

public:
	ATrueSkyWater(const FObjectInitializer& ObjectInitializer);

	/** Update and render this water object*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool Render;

	/** Is a boundless ocean (Only one can exist at a time)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool boundlessOcean;

	/** Set how rough the water is via approximate Beaufort value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float BeaufortScale;

	/** Set the direction the wind is blowing in*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindDirection;

	/** Set the water's dependancy on the wind direction*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindDependency;

	/** Use preset Scattering/Absorption values*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties|Colour")
	bool UseColourPresets;

	/** Colour presets*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties|Colour")
	EWaterColour ColourPresets;

	/** Set the Scattering coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties|Colour", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector Scattering;

	/** Set the Absorption coefficient of the water*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties|Colour", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector Absorption;

	/** Set the resolution of the profile buffers. Must be in powers of 2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "512", ClampMax = "4096"))
	int ProfileBufferResolution;

	/** Set the dimensions of the water object*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Properties, meta = (ClampMin = "0.0"))
	FVector Dimension;

	/** Set the location of the water object*/
	FVector Location;

	/** Set the Rotation of the water object*/
	float Rotation;

	/** Enable advanced water surface options to allow for finer tuning*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Surface Generation")
	bool AdvancedSurfaceOptions;

	/** Set the wind speed value*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Surface Generation", meta = (ClampMin = "0.0", ClampMax = "40.0"))
	float WindSpeed;

	/** Set the Amplitude of the water waves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Surface Generation", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float WaveAmplitude;

	/** Set the Maximum Wavelength of the water waves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Surface Generation", meta = (ClampMin = "1.01", ClampMax = "100.0"))
	float MaxWavelength;

	/** Set the Minimum Wavelength of the water waves*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Surface Generation", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MinWavelength;

	/** Enable the ocean wave grid*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Wave Grid")
	bool EnableWaveGrid;

	/** Set how far the wave grid extends to*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Wave Grid", meta = (ClampMin = "64", ClampMax = "256"))
	int WaveGridDistance;

	/** Enable foam to be generated and rendered*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Foam")
	bool EnableFoam;

	/** Set the level at which surface foam will start to be generated (Ocean only)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Foam", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float FoamStrength;

	/** Update the shore depth every frame rather than once at startup*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Shore Effects")
	bool EnableShoreEffects;

	/** Assign a SceneCapture2D actor to write the depth of the surface underneath the water.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Shore Effects")
	class ASceneCapture2D* ShoreDepthSceneCapture2D;

	/** Width of the shore depth texture*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Shore Effects")
	float ShoreDepthWidth;

	/** Extent of the shore depth texture*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced|Shore Effects")
	float ShoreDepthExtent;

	static void InitializeTrueSkyWaterPropertyMap();

	void PostRegisterAllComponents() override;

	bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

	void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TrueSky|Water")
	void SetWaterFloat(ETrueSkyWaterPropertiesFloat Property, float Value);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Water")
	void SetWaterInt(ETrueSkyWaterPropertiesInt Property, int Value);
	UFUNCTION(BlueprintCallable, Category = "TrueSky|Water")
	void SetWaterVector(ETrueSkyWaterPropertiesVector Property, FVector Value);

	void ApplyRenderingOptions();

private:

	void PostInitProperties() override;

	void PostLoad() override;

	void PostInitializeComponents() override;

	void Destroyed() override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
		
	void TransferProperties();

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void EditorApplyTranslation(const FVector & DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;

	virtual bool CanEditChange(const UProperty* InProperty)const override;
#endif
	UPROPERTY(NonPIEDuplicateTransient)
	int ID;

	bool initialised;

public:
	static int32 TSWaterPropertiesFloatIdx;
	static int32 TSWaterPropertiesIntIdx;
	static int32 TSWaterPropertiesVectorIdx;

	// Enumeration names.
	typedef TMap< int64, TrueSkyPropertyFloatData > TrueSkyWaterPropertyFloatMap;
	static TArray< TrueSkyWaterPropertyFloatMap > TrueSkyWaterPropertyFloatMaps;
	typedef TMap< int64, TrueSkyPropertyIntData > TrueSkyWaterPropertyIntMap;
	static TArray< TrueSkyWaterPropertyIntMap > TrueSkyWaterPropertyIntMaps;
	typedef TMap< int64, TrueSkyPropertyVectorData > TrueSkyWaterPropertyVectorMap;
	static TArray< TrueSkyWaterPropertyVectorMap > TrueSkyWaterPropertyVectorMaps;

protected:
	static void PopulateWaterPropertyFloatMap(UEnum* EnumerationType, TrueSkyWaterPropertyFloatMap& EnumerationMap);
	static void PopulateWaterPropertyIntMap(UEnum* EnumerationType, TrueSkyWaterPropertyIntMap& EnumerationMap);
	static void PopulateWaterPropertyVectorMap(UEnum* EnumerationType, TrueSkyWaterPropertyVectorMap& EnumerationMap);
	//class UTrueSkyWaterComponent* GetWaterComponent() const;
};