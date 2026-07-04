// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CWeatherDataType.h"
#include "CesiumGeoreference.h"
#include "CWeatherBPFLibrary.generated.h"

USTRUCT(BlueprintType)
struct FMMoonData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Elevation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Azimuth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float RotationAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float DistanceToTheMoon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float LibrationInLatitude;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float LibrationInLongitude;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float PositionAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float DiskIllumination;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Declination;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float RightAscension;
};
USTRUCT(BlueprintType)
struct FMSunData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Elevation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Azimuth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Distance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float Declination;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Position Plugin")
		float RightAscension;
};

/**
 *
 */
UCLASS()
class AWEATHERSYSTEM_API UCWeatherBPFLibrary : public UBlueprintFunctionLibrary
{
public:
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "AWeatherBPFLibrary")
		static void SetNetSyncTime(int32 InSyncTime);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static int32 GetNetSyncTime();
	UFUNCTION(BlueprintCallable, Category = "AWeatherBPFLibrary")
		static void SetGSkyAtmosphereInfo(const FSkyAtmosphereInfo& InCloudInfo);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FSkyAtmosphereInfo GetGSkyAtmosphereInfo();
	UFUNCTION(BlueprintCallable, Category = "AWeatherBPFLibrary")
		static void SetGCloudStatInfo(const FCloudStatInfo& InCloudInfo);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FCloudStatInfo GetGCloudStatInfo();
	UFUNCTION(BlueprintCallable, Category = "AWeatherBPFLibrary")
		static void SetGWeatherInfo(const FWeatherInfo& InWeatherInfo);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FWeatherInfo GetGWeatherInfo();

		//输入日期+N天后日期
	UFUNCTION(BlueprintCallable, Category = "AWeatherBPFLibrary")
		static void CalcDateAddN(int32 InYear, int32 InMonth, int32 InDay, int32 AddNDay, int32& OutYear, int32& OutMonth, int32& OutDay);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static bool CheckUEWeatherValidTag(int32 InVal, UE_WEATHER_VALID_TAG InCheckTag);
	UFUNCTION(BlueprintCallable, category = "AWeatherBPFLibrary")
		static void PGetHMSFromSolarTime(float SolarTime, int32& Hour, int32& Minute, int32& Second);

	//--------------------------------------------------------SKY Position--------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Sky Position Plugin")
		static void GetMoonPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FMMoonData& MoonData);
	UFUNCTION(BlueprintCallable, Category = "Sky Position Plugin")
		static void GetSunPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FMSunData& SunData);
	UFUNCTION(BlueprintCallable, Category = "Sky Position Plugin")
		static void GetStarsPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FRotator& StarSphereRotator);
	UFUNCTION(BlueprintPure, Category = "Sky Position Plugin")
		static void CalculateMoonRotationSPP(float LibrationInLatitude, float LibrationInLongitude, float PositionAngle, float RotationAngle, FRotator& MoonRotator);

	UFUNCTION(BlueprintPure, category = "AWeatherBPFLibrary")
		static float CalcDeltaAngle(const FVector& InAngle, const FVector& NormalUp, const FVector& NormalForward);
	UFUNCTION(BlueprintPure, category = "AWeatherBPFLibrary")
		static FRotator CalcSolarTimeRot(float SolarTime, float Latitude, float Longitude, float TimeZone, int32 Year, int32 Month, int32 Day);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FRotator TransformRotationENUtoUe(ACesiumGeoreference* GeoReference, FVector UeLoc, FRotator EnuRot);
	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FVector TransformLonLatHeightToUe(ACesiumGeoreference* GeoReference, FString Lon, FString Lat, FString Height);

	UFUNCTION(BlueprintPure, Category = "AWeatherBPFLibrary")
		static FMatrix GetAbsUeToEcefMatrix(ACesiumGeoreference* InGeoRef);
};
