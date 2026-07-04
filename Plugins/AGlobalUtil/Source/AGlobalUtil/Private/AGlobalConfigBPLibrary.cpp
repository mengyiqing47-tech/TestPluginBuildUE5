// Fill out your copyright notice in the Description page of Project Settings.

#include "AGlobalConfigBPLibrary.h" // 本类头文件
#include "Misc/Paths.h" // FPaths 路径工具
#include "Misc/ConfigCacheIni.h" // GConfig 全局 INI 配置缓存
#include "GenericPlatform/GenericPlatformFile.h" // IPlatformFile 平台文件接口
#include "HAL/PlatformFilemanager.h" // FPlatformFileManager 平台文件管理器


void UGlobalConfigBPLibrary::GetConfigBool(FString tSection, FString tKey, FString tPath, bool& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径: Content目录 + 相对文件路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile(); // 获取平台文件访问接口
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查 INI 配置文件是否存在
		retCode = -1; // 配置文件不存在时返回错误码 -1
	if (!GConfig)return; // 全局配置系统未初始化则直接退出
	GConfig->Flush(true, filePath); // 刷新配置缓存，从磁盘重新读取 INI 文件
	retCode = GConfig->GetBool(*tSection, *tKey, retValue, filePath); // 读取指定位段键名的布尔类型值，返回 >0 表示成功
}

void UGlobalConfigBPLibrary::SetConfigBool(FString tSection, FString tKey, bool tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return; // 全局配置系统未初始化则直接退出
	GConfig->SetBool(*tSection, *tKey, tValue, filePath); // 写入布尔值到配置缓存
	GConfig->Flush(false, filePath); // 刷新配置缓存，将内存数据写入磁盘 INI 文件
	retCode = 0; // 写入成功返回 0
}

void UGlobalConfigBPLibrary::GetConfigString(FString tSection, FString tKey, FString tPath, FString& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile(); // 获取平台文件访问接口
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查 INI 配置文件是否存在
		retCode = -1; // 配置文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 从磁盘重新读取 INI
	retCode = GConfig->GetString(*tSection, *tKey, retValue, filePath); // 读取字符串类型值
}

void UGlobalConfigBPLibrary::SetConfigString(FString tSection, FString tKey, FString tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetString(*tSection, *tKey, *tValue, filePath); // 写入字符串到配置缓存
	GConfig->Flush(false, filePath); // 将缓存持久化写入磁盘
	retCode = 0; // 写入成功
}

void UGlobalConfigBPLibrary::GetConfigText(FString tSection, FString tKey, FString tPath, FText& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 刷新缓存，重新读取磁盘文件
	retCode = GConfig->GetText(*tSection, *tKey, retValue, filePath); // 读取 FText 类型值（支持本地化字符串）
}

void UGlobalConfigBPLibrary::SetConfigText(FString tSection, FString tKey, FText tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetText(*tSection, *tKey, tValue, filePath); // 写入 FText 到配置缓存
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigFloat(FString tSection, FString tKey, FString tPath, float& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
	{
		retCode = -1; // 文件不存在时返回错误码 -1
	}
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetFloat(*tSection, *tKey, retValue, filePath); // 读取浮点数类型值
}

void UGlobalConfigBPLibrary::SetConfigFloat(FString tSection, FString tKey, float tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetFloat(*tSection, *tKey, tValue, filePath); // 写入浮点数
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigInt(FString tSection, FString tKey, FString tPath, int32& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetInt(*tSection, *tKey, retValue, filePath); // 读取 int32 类型值
}

void UGlobalConfigBPLibrary::SetConfigInt(FString tSection, FString tKey, int32 tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetInt(*tSection, *tKey, tValue, filePath); // 写入 int32
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigVector(FString tSection, FString tKey, FString tPath, FVector& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetVector(*tSection, *tKey, retValue, filePath); // 读取 FVector 三维向量值（X,Y,Z）
}

void UGlobalConfigBPLibrary::SetConfigVector(FString tSection, FString tKey, FVector tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetVector(*tSection, *tKey, tValue, filePath); // 写入 FVector
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigVector2D(FString tSection, FString tKey, FString tPath, FVector2D& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetVector2D(*tSection, *tKey, retValue, filePath); // 读取 FVector2D 二维向量值（X,Y）
}

void UGlobalConfigBPLibrary::SetConfigVector2D(FString tSection, FString tKey, FVector2D tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetVector2D(*tSection, *tKey, tValue, filePath); // 写入 FVector2D
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigVector4(FString tSection, FString tKey, FString tPath, FVector4& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetVector4(*tSection, *tKey, retValue, filePath); // 读取 FVector4 四维向量值（X,Y,Z,W）
}

void UGlobalConfigBPLibrary::SetConfigVector4(FString tSection, FString tKey, FVector4 tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetVector4(*tSection, *tKey, tValue, filePath); // 写入 FVector4
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigColor(FString tSection, FString tKey, FString tPath, FColor& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetColor(*tSection, *tKey, retValue, filePath); // 读取 FColor 颜色值（R,G,B,A）
}

void UGlobalConfigBPLibrary::SetConfigColor(FString tSection, FString tKey, FColor tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetColor(*tSection, *tKey, tValue, filePath); // 写入 FColor
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigRotator(FString tSection, FString tKey, FString tPath, FRotator& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetRotator(*tSection, *tKey, retValue, filePath); // 读取 FRotator 旋转值（Pitch,Yaw,Roll）
}

void UGlobalConfigBPLibrary::SetConfigRotator(FString tSection, FString tKey, FRotator tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetRotator(*tSection, *tKey, tValue, filePath); // 写入 FRotator
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

void UGlobalConfigBPLibrary::GetConfigArray(FString tSection, FString tKey, FString tPath, TArray<FString>& retValue, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!(platformFile.FileExists(filePath.GetCharArray().GetData()))) // 检查文件是否存在
		retCode = -1; // 文件不存在时返回错误码 -1
	if (!GConfig)return;
	GConfig->Flush(true, filePath); // 重新读取磁盘 INI
	retCode = GConfig->GetArray(*tSection, *tKey, retValue, filePath); // 读取字符串数组值
}

void UGlobalConfigBPLibrary::SetConfigArray(FString tSection, FString tKey, TArray<FString> tValue, FString tPath, int32& retCode)
{
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + tPath); // 拼接完整路径
	if (!GConfig)return;
	GConfig->SetArray(*tSection, *tKey, tValue, filePath); // 写入字符串数组
	GConfig->Flush(false, filePath); // 持久化写入磁盘
	retCode = 0;
}

// --- 带默认值的初始化读取函数: 读取失败时自动写入默认值并返回该默认值 ---

int32 UGlobalConfigBPLibrary::GetConfigInitInt(FString tSection, FString tKey, FString tPath, int32 tDefaultValue)
{
	int RetVal, RetCode; // 返回值、返回码
	GetConfigInt(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 int32 配置值
	if (RetCode > 0) // 读取成功 (RetCode > 0)
		return RetVal;
	else // 读取失败：文件不存在(-1)或其他错误
	{
		SetConfigInt(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值到 INI 文件
		return tDefaultValue; // 返回默认值
	}
}

float UGlobalConfigBPLibrary::GetConfigInitFloat(FString tSection, FString tKey, FString tPath, float tDefaultValue)
{
	int RetCode;
	float RetVal;
	GetConfigFloat(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 float 配置值
	if (RetCode > 0) // 读取成功
		return RetVal;
	else
	{
		SetConfigFloat(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FString UGlobalConfigBPLibrary::GetConfigInitString(FString tSection, FString tKey, FString tPath, FString tDefaultValue)
{
	int RetCode;
	FString RetVal;
	GetConfigString(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FString 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigString(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}


bool UGlobalConfigBPLibrary::GetConfigInitBool(FString tSection, FString tKey, FString tPath, bool tDefaultValue)
{
	int RetCode;
	bool RetVal;
	GetConfigBool(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 bool 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigBool(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FColor UGlobalConfigBPLibrary::GetConfigInitColor(FString tSection, FString tKey, FString tPath, FColor tDefaultValue)
{
	int RetCode;
	FColor RetVal;
	GetConfigColor(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FColor 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigColor(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FRotator UGlobalConfigBPLibrary::GetConfigInitRotator(FString tSection, FString tKey, FString tPath, FRotator tDefaultValue)
{
	int RetCode;
	FRotator RetVal;
	GetConfigRotator(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FRotator 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigRotator(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FVector4 UGlobalConfigBPLibrary::GetConfigInitVector4(FString tSection, FString tKey, FString tPath, FVector4 tDefaultValue)
{
	int RetCode;
	FVector4 RetVal;
	GetConfigVector4(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FVector4 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigVector4(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FVector UGlobalConfigBPLibrary::GetConfigInitVector(FString tSection, FString tKey, FString tPath, FVector tDefaultValue)
{
	int RetCode;
	FVector RetVal;
	GetConfigVector(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FVector 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigVector(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FVector2D UGlobalConfigBPLibrary::GetConfigInitVector2D(FString tSection, FString tKey, FString tPath, FVector2D tDefaultValue)
{
	int RetCode;
	FVector2D RetVal;
	GetConfigVector2D(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FVector2D 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigVector2D(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

FText UGlobalConfigBPLibrary::GetConfigInitText(FString tSection, FString tKey, FString tPath, FText tDefaultValue)
{
	int RetCode;
	FText RetVal;
	GetConfigText(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取 FText 配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigText(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}

TArray<FString> UGlobalConfigBPLibrary::GetConfigInitArray(FString tSection, FString tKey, FString tPath, TArray<FString> tDefaultValue)
{
	int RetCode;
	TArray<FString> RetVal;
	GetConfigArray(tSection, tKey, tPath, RetVal, RetCode); // 尝试读取字符串数组配置值
	if (RetCode > 0)
		return RetVal;
	else
	{
		SetConfigArray(tSection, tKey, tDefaultValue, tPath, RetCode); // 写入默认值
		return tDefaultValue;
	}
}