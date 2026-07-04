// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/Paths.h"
#include "AGlobalUtilBPLibrary.generated.h"

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/

struct FFileChangeData;

UENUM(BlueprintType)
enum class EPensorImageFormat : uint8
{
	/** Invalid or unrecognized format. */
	Invalid = 255,

	/** Portable Network Graphics. */
	PNG = 0,

	/** Joint Photographic Experts Group. */
	JPEG,

	/** Single channel JPEG. */
	GrayscaleJPEG,

	/** Windows Bitmap. */
	BMP,

	/** Windows Icon resource. */
	ICO,

	/** OpenEXR (HDR) image file format. */
	EXR,

	/** Mac icon. */
	ICNS
};

UENUM(BlueprintType)
enum class EPFileChangeAction : uint8
{
	FCA_Unknown,
	FCA_Added,
	FCA_Modified,
	FCA_Removed
};

UENUM(BlueprintType)
enum class EUtilRGBFormat : uint8
{
	Invalid = 0,
	RGBA = 1,
	BGRA = 2,
	Gray = 3
};


USTRUCT(BlueprintType)
struct FPImgColorData
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "PImgColorData")
		int32 Width = 1024;
	UPROPERTY(BlueprintReadWrite, Category = "PImgColorData")
		int32 Height = 1024;
	UPROPERTY(BlueprintReadWrite, Category = "PImgColorData")
		TArray<uint8> ImgData;
	UPROPERTY(BlueprintReadWrite, Category = "PImgColorData")
		EUtilRGBFormat ImgPixelFmt = EUtilRGBFormat::BGRA;
};


struct FPCImgColorData
{
	int32 Width = 1024;
	int32 Height = 1024;
	uint8* ImgData = nullptr;
	EUtilRGBFormat ImgPixelFmt = EUtilRGBFormat::BGRA;
};

USTRUCT(BlueprintType)
struct FPFileChangeData
{
	GENERATED_BODY()
	FPFileChangeData()
	{
		FPaths::MakeStandardFilename(Filename);
	}
	FPFileChangeData(const FFileChangeData& Data);

	FPFileChangeData(const FString& InFilename, EPFileChangeAction InAction)
		: Filename(InFilename)
		, Action(InAction)
	{
		FPaths::MakeStandardFilename(Filename);
	}
	UPROPERTY(BlueprintReadWrite, Category = "PFileChangeData")
		FString Filename;
	UPROPERTY(BlueprintReadWrite, Category = "PFileChangeData")
		EPFileChangeAction Action = EPFileChangeAction::FCA_Unknown;
};


USTRUCT(BlueprintType)
struct FPFileChangeDelegateInfo
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "PFileChangeDelegateInfo")
		FString DelegateName;
	UPROPERTY(BlueprintReadWrite, Category = "PFileChangeDelegateInfo")
		FString WatcherPath;
	FORCEINLINE bool operator == (const FPFileChangeDelegateInfo& Other) const
	{
		return (DelegateName == Other.DelegateName && WatcherPath == Other.WatcherPath);
	}

	FORCEINLINE bool operator != (const FPFileChangeDelegateInfo& Other) const
	{
		return !(*this==Other);
	}

	friend FORCEINLINE uint32 GetTypeHash(const FPFileChangeDelegateInfo& InKey) 
	{
		uint32 KeyHash = 0;
		KeyHash = HashCombine(KeyHash, GetTypeHash(InKey.DelegateName));
		KeyHash = HashCombine(KeyHash, GetTypeHash(InKey.WatcherPath));
		return KeyHash;
	}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FPDirectoryChanged, const TArray<FPFileChangeData>&, FileChangeLists);

UCLASS()
class AGLOBALUTIL_API UAGlobalUtilBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	//------------------------------------------------
	//----------------------CommonUtil----------------
	//------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "MsgLog")
		static void ShowMsgLog(FString Msg);


	//------------------------------------------------
	//----------------------Hash----------------
	//------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "MsgLog")
		static FString CalculateFileMD5(const FString& FilePath, int32 ChunkSize=8192);


	//------------------------------------------------
	//----------------------BitOpr----------------
	//------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "BitOpr")
		static void SetBit8(uint8 InVal, int32 InBit, bool InStat);
	UFUNCTION(BlueprintCallable, Category = "BitOpr")
		static void SetBit32(int32 InVal, int32 InBit, bool InStat);
	UFUNCTION(BlueprintPure, Category = "BitOpr")
		static bool GetBit8(uint8 InVal, int32 InBit);
	UFUNCTION(BlueprintPure, Category = "BitOpr")
		static bool GetBit32(int32 InVal, int32 InBit);
	//¥””“µΩ◊Û≈≈–Ú
	UFUNCTION(BlueprintPure, Category = "BitOpr")
		static TArray<bool> GetBitAll8(uint8 InVal);
	//¥””“µΩ◊Û≈≈–Ú
	UFUNCTION(BlueprintPure, Category = "BitOpr")
		static TArray<bool> GetBitAll32(int32 InVal);

	//------------------------------------------------
	//----------------------ImgOpr--------------------
	//------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "ImgOpr")
		static EPensorImageFormat GetImageFormatByPath(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "ImgOpr")
		static bool SaveRT(UTextureRenderTarget2D* RT, const FString& SavePath);

	UFUNCTION(BlueprintCallable, Category = "ImgOpr")
		static bool SaveColorData(FPImgColorData Data, const FString& SavePath);
	static bool CSaveColorData(FPCImgColorData Data, const FString& SavePath);

	UFUNCTION(BlueprintCallable, Category = "ImgOpr")
		static UTexture2D* LoadImgToTexture2D(const FString& FilePath);

	//------------------------------------------------
	//----------------------FileOpr-------------------
	//------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "FileOpr")
		static void ListDir(const FString& DirPath, TArray<FString>& OutFiles);
	UFUNCTION(BlueprintPure, Category = "FileOpr")
		static bool DeleteFile(const FString& FilePath);
	UFUNCTION(BlueprintCallable, Category = "FileOpr")
		static void RegisterDirectoryWatcher(const FPFileChangeDelegateInfo& Info, FPDirectoryChanged DirectoryChangedDelegate);
	UFUNCTION(BlueprintCallable, Category = "FileOpr")
		static void UnRegisterDirectoryWatcher(const FPFileChangeDelegateInfo& Info);
	static TMap<FPFileChangeDelegateInfo, FDelegateHandle> DirectoryWatcherHandleMap;

	//------------------------------------------------
	//----------------------Date-------------------
	//------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "DateOpr")
		static FString GetNowDateStr(const FString& Format = TEXT("%Y%m%d%H%M%S%s"));

	//------------------------------------------------
	//----------------------GetUtil-------------------
	//------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "GetUtil")
		static FIntVector BPGetWorldOrigin();
	UFUNCTION(BlueprintPure, Category = "GetUtil")
		static FTransform BPGetViewpointTransform();
	UFUNCTION(BlueprintPure, Category = "GetUtil")
		static FTransform BPGetPlayerPawnTransform();


	//------------------------------------------------
	//----------------------Curve---------------------
	//------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "PCurves")
		static void AddCurveFloatPoint(UPARAM(ref) FInterpCurveFloat& Curve, float InVal, float OutVal);
	UFUNCTION(BlueprintCallable, Category = "PCurves")
		static void ClearCurveFloatPoint(UPARAM(ref) FInterpCurveFloat& Curve);
	UFUNCTION(BlueprintPure, Category = "PCurves")
		static float GetCurveFloatValue(UPARAM(ref) FInterpCurveFloat& Curve, float InVal, float DefaultVal);
	UFUNCTION(BlueprintCallable, Category = "PCurves")
		static void AddCurveVectorPoint(UPARAM(ref) FInterpCurveVector& Curve, float InVal, FVector OutVal);
	UFUNCTION(BlueprintCallable, Category = "PCurves")
		static void ClearCurveVectorPoint(UPARAM(ref) FInterpCurveVector& Curve);
	UFUNCTION(BlueprintPure, Category = "PCurves")
		static FVector GetCurveVectorValue(UPARAM(ref) FInterpCurveVector& Curve, float InVal, FVector DefaultVal);
};
