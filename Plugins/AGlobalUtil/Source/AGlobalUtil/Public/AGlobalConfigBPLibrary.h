// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGlobalConfigBPLibrary.generated.h"

/**
 * 函数库，提供配置文件操作
 */
UCLASS()
class AGLOBALUTIL_API UGlobalConfigBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigBool(FString tSection, FString tKey, FString tPath, bool& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigBool(FString tSection, FString tKey, bool tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigString(FString tSection, FString tKey, FString tPath, FString& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigString(FString tSection, FString tKey, FString tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigText(FString tSection, FString tKey, FString tPath, FText& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigText(FString tSection, FString tKey, FText tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigFloat(FString tSection, FString tKey, FString tPath, float& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigFloat(FString tSection, FString tKey, float tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigInt(FString tSection, FString tKey, FString tPath, int32& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigInt(FString tSection, FString tKey, int32 tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigVector(FString tSection, FString tKey, FString tPath, FVector& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigVector(FString tSection, FString tKey, FVector tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigVector2D(FString tSection, FString tKey, FString tPath, FVector2D& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigVector2D(FString tSection, FString tKey, FVector2D tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigVector4(FString tSection, FString tKey, FString tPath, FVector4& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigVector4(FString tSection, FString tKey, FVector4 tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigColor(FString tSection, FString tKey, FString tPath, FColor& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigColor(FString tSection, FString tKey, FColor tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigRotator(FString tSection, FString tKey, FString tPath, FRotator& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigRotator(FString tSection, FString tKey, FRotator tValue, FString tPath, int32& retCode);
	UFUNCTION(BlueprintPure, Category = "INIConfig")
		static void GetConfigArray(FString tSection, FString tKey, FString tPath, TArray<FString>& retValue, int32& retCode);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static void SetConfigArray(FString tSection, FString tKey, TArray<FString> tValue, FString tPath, int32& retCode);
	//---------------------------------------------------------获取配置文件值，获取失败设置并返回默认值-------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static int32 GetConfigInitInt(FString tSection, FString tKey, FString tPath, int32 tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static float GetConfigInitFloat(FString tSection, FString tKey, FString tPath, float tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FString GetConfigInitString(FString tSection, FString tKey, FString tPath, FString tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static bool GetConfigInitBool(FString tSection, FString tKey, FString tPath, bool tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FColor GetConfigInitColor(FString tSection, FString tKey, FString tPath, FColor tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FRotator GetConfigInitRotator(FString tSection, FString tKey, FString tPath, FRotator tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FVector4 GetConfigInitVector4(FString tSection, FString tKey, FString tPath, FVector4 tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FVector GetConfigInitVector(FString tSection, FString tKey, FString tPath, FVector tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FVector2D GetConfigInitVector2D(FString tSection, FString tKey, FString tPath, FVector2D tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static FText GetConfigInitText(FString tSection, FString tKey, FString tPath, FText tDefaultValue);
	UFUNCTION(BlueprintCallable, Category = "INIConfig")
		static TArray<FString> GetConfigInitArray(FString tSection, FString tKey, FString tPath, TArray<FString> tDefaultValue);
};
