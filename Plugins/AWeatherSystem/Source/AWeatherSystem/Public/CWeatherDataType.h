// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CWeatherDataType.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPWeatherSys, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimTimeUpdate,float,SimTime);

UENUM(BlueprintType)
enum class ECloudType : uint8
{
	CUMULUS,//积云
	FLOCCUS,//絮状云
	LENTICULARIS,//荚状云
	FILWISE,//丝状云
};

//天气帧标识
UENUM(BlueprintType)
enum class UE_WEATHER_VALID_TAG : uint8
{
	//太阳强度
	ESUN_INT,
	//天光强度
	ESKY_LIGHT_INT,
	//星星亮度
	ESTAR_INT,
	//云底高
	ECLOUD_BTM,
	//云高度
	ECLOUD_HEIGHT,
	//云密度
	ECLOUD_DENSITY,
	//云覆盖距离
	ECLOUD_COVER,
	//云减淡距离
	ECLOUD_ATTENUATION,
	//风力
	EWIND_INT,
	//风向
	EWIND_DIRECT,
	//雨级别
	ERAIN_LEVEL,
	//雪级别
	ESNOW_LEVEL,
	//积水程度
	EWET_COVER,
	//积雪程度
	ESNOW_COVER,
	//结冰程度
	EICE_COVER,
	//能见度
	EVISIBILITY,
	//雾浓度
	EFOGDENSITY,
	//天空雾
	ESKYFOG,
};

//云状态信息
USTRUCT(BlueprintType)
struct FCloudStatInfo
{
	GENERATED_BODY()
public:
	//云类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		ECloudType Type = ECloudType::CUMULUS;
};

//天气状态信息
USTRUCT(BlueprintType)
struct FWeatherInfo
{
	GENERATED_BODY()
public:
	//网络同步时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		int32 NetSyncTime=0;
	//闪电队列，0无闪电 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		TArray<int32> LightningArray;
	//模拟时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float SimTime=0.0f;
};

//天空大气信息
USTRUCT(BlueprintType)
struct FSkyAtmosphereInfo
{
	GENERATED_BODY()
public:
	//开始调整大气范围阈值 Km
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float InscribedGroundThreshold = -20;
	//结束调整大气范围阈值 Km
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float CircumscribedGroundThreshold = 100;
	//最小地形高度 Km
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float MinBottomRadius = 6365;
	//最大地形高度 Km
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float MaxBottomRadius = 6387;
};

USTRUCT(BlueprintType)
struct FWeatherFrame
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fTime;                             //时刻
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fSunIntensity;                     //太阳强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fSkyLightIntensity;                //天光强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fStarIntensity;                    //星星亮度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudBtm;                         //云底高 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudHeight;                      //云高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudDensity;                     //云密度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudCover;                       //云覆盖距离，公里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudAttenuation;                 //云减淡距离，公里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fWindIntensity;                    //风力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FVector						  WindDirect;                         //风向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                         nRainLevel;                        //雨级别
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                         nSnowLevel;                        //雪级别
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                       nWetCover;                         //积水程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                       nSnowCover;                        //积雪程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                       nIceCover;                         //结冰程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fVisibility;                       //能见度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                         nFogDensity;                       //雾浓度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8                         nSkyFog;							//天空雾
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		int32                       lValidTag;                         //有效性标识 Ref：WEATHER_VALID_TAG
	FWeatherFrame() {}
};


USTRUCT(BlueprintType)
struct FWeatherCurveFrame
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fTime;                             //时刻
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fSunIntensity;                     //太阳强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fSkyLightIntensity;                //天光强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fStarIntensity;                    //星星亮度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudBtm;                         //云底高 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudHeight;                      //云高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudDensity;                     //云密度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudCover;                       //云覆盖距离，公里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fCloudAttenuation;                 //云减淡距离，公里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fWindIntensity;                    //风力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FVector						  WindDirect;                         //风向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         nRainLevel;                        //雨级别
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         nSnowLevel;                        //雪级别
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                       nWetCover;                         //积水程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                       nSnowCover;                        //积雪程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                       nIceCover;                         //结冰程度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fVisibility;                       //能见度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fFogDensity;                       //雾浓度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fSkyFog;							//天空雾
};


USTRUCT(BlueprintType)
struct FWeatherArea
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fOriginLon;                        //中心经度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fOriginLat;                        //中心纬度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float                         fRadius;                           //半径 cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		TArray<FWeatherFrame>         datas;                             //天气帧数据
	FWeatherArea() {}
};


USTRUCT(BlueprintType)
struct FWeatherCurves
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float OriginLon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float OriginLat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float Radius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat SunIntensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat SkyLightIntensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat StarIntensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat CloudBtm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat CloudHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat CloudDensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat CloudCover;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat CloudAttenuation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat WindIntensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveVector WindDirect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat RainLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat SnowLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat FogDensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat WetCover;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat SnowCover;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat IceCover;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		FInterpCurveFloat SkyFog;
};


USTRUCT(BlueprintType)
struct FTimeControlData
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float fTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float fTimeProgressionScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float fTimeLoopStart;//时间循环开始 11.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float fTimeLoopEnd;//时间循环结束 13.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8 ValueTag;//有效性标识 111 循环 进度 时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		bool bTimeLoop;//是否开启时间循环

	FTimeControlData() {}
};

USTRUCT(BlueprintType)
struct FDateControlData
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		int32 Year;//年
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8 Month;//月
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		uint8 Day;//日
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PWeatherSys")
		float TimeZone;//时区

	FDateControlData() {}
};

UCLASS(BlueprintType, Blueprintable)
class UCLoudTypeDataAsset :public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UCloudTypeDataAsset")
		FLinearColor CloudNoiseTile3D;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UCloudTypeDataAsset")
		FLinearColor CloudNoiseTile3DTop;
};