// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWeatherDataType.h"
#include "CWeatherSystem.generated.h"

class ACWeatherSystem;

namespace PWeatherSys {
	AWEATHERSYSTEM_API extern ACWeatherSystem* pGCWeatherSystem;
	//SkyAtmosphere
	AWEATHERSYSTEM_API extern FSkyAtmosphereInfo GSkyAtmosphereInfo;//天空大气信息
	//CloudStatInfo
	AWEATHERSYSTEM_API extern FCloudStatInfo GCloudStatInfo;
	//WeatherInfo
	AWEATHERSYSTEM_API extern FWeatherInfo GWeatherInfo;
}

UCLASS()
class AWEATHERSYSTEM_API ACWeatherSystem : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	ACWeatherSystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Time")
		bool P2_SetTime(const FTimeControlData& TimeData);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Time")
		bool P2_SetDate(const FDateControlData& DateData);
	//设置时段 0 黎明 1白天 2傍晚 3黑夜
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Time")
		bool P2_SetTimeSection(uint8 TimeSection);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool P2_SetWeatherAreaData(UPARAM(ref) TArray<FWeatherArea>& InWeatherData);
	UFUNCTION(BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		void P2_ApplyWorldOffset(const FVector& InOffset, bool bWorldShift);

	UFUNCTION(BlueprintCallable, Category = "WeatherTimeManager|Weather")
		bool SaveProfile(FString InFileName);
	UFUNCTION(BlueprintCallable, Category = "WeatherTimeManager|Weather")
		bool LoadProfile(FString InFileName);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool SetCloudType(ECloudType InType);
	UFUNCTION(BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool SetSeaStatLevel(uint8 Level);
	UFUNCTION(BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool SetMoonLightIntensity(float Intensity);
	const TArray<FWeatherArea>& GetWeatherData();
	bool SetWeatherData(TArray<FWeatherArea>& data);
	bool SetWeatherSequenceAssetByRef(FString SequenceAssetPath);

	UFUNCTION(BlueprintCallable, Category = "WeatherTimeManager|Time")
		float GetSomedayFloat(int iYear, int iMonth, int iDay);
	UFUNCTION(BlueprintCallable, Category = "WeatherTimeManager|Atmosphere")
		void UpdateAtmosphereRadius();
	UFUNCTION(BlueprintCallable, Category = "WeatherTimeManager|Atmosphere")
		void SetSkyAtmosphereGroundRadius(USkyAtmosphereComponent* Sky, float Radius);

	UPROPERTY(BlueprintReadWrite, Category = "WeatherTimeManager | Weather")
		TArray<uint8> LightCountBuffer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager")
		bool bEnableDebugInfo;
	//天气数据
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Weather")
		TArray<FWeatherArea> WeatherData;
	//所有区域天气曲线，用于计算CurFrame
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere,  Category = "WeatherTimeManager | Weather")
		TArray< FWeatherCurves> WeatherCurves;
	//当前帧数据 UpdateWeather使用此数据
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "WeatherTimeManager | Weather")
		FWeatherCurveFrame CurFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Atmosphere", meta = (EditCondition = "bDebugSkyAtmosphereInfo", EditConditionHides))
		FSkyAtmosphereInfo SkyAtmosphereInfo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Atmosphere")
		bool bDebugSkyAtmosphereInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Atmosphere")
		float SkyAtmosphereBottomRadiusOffset = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Atmosphere")
		class USkyAtmosphereComponent* SkyAtmosphereComp = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager | Atmosphere")
		class USceneComponent* RootComp = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeatherTimeManager")
		class ACesiumGeoreference* pGeoRef = nullptr;
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "WeatherTimeManager")
		FOnSimTimeUpdate OnSimTimeUpdate;
public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool SetStarParams(float Low0 = 0.0f, float Height0 = 1.0f, float Low1 = 0.0f, float Height1 = 1.0f);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "WeatherTimeManager|Weather")
		bool ReCaptureSkyLight();
};
