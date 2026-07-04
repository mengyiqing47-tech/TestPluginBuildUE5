// Fill out your copyright notice in the Description page of Project Settings.


#include "CWeatherSystem.h" // 天气系统主 Actor 头文件
#include "Json.h" // UE JSON 序列化/反序列化
#include "CesiumGeoreference.h" // Cesium 地理参考 Actor（经纬高 ↔ UE 坐标）
#include "CesiumGlobeAnchorComponent.h" // Cesium 地球锚点组件
#include "Components/SkyAtmosphereComponent.h" // USkyAtmosphereComponent 天空大气渲染组件
#include "AGlobalUtilBPLibrary.h" // 全局工具库（日志/文件/位操作/视点查询）
#include "Kismet/KismetMathLibrary.h" // 蓝图数学工具（线性插值等）

namespace PWeatherSys {
	ACWeatherSystem* pGCWeatherSystem = nullptr; // 全局天气系统单例指针 — 在 BeginPlay() 时赋值
	FSkyAtmosphereInfo GSkyAtmosphereInfo; // 天空大气高度阈值、半径配置
	FCloudStatInfo GCloudStatInfo; // 云类型等状态信息
	FWeatherInfo GWeatherInfo; // 天气状态信息（含网络同步时间、闪电队列、模拟时间）
}

// 数学转换工具命名空间：UE 向量 ↔ GLM 向量互转
namespace PWSMathConv {
	glm::dvec3 UeToGlm(FVector InVector){
		return glm::dvec3(InVector.X, InVector.Y, InVector.Z); // FVector → glm::dvec3 转换（按分量 X,Y,Z 构造）
	}
	FVector GlmToUe(glm::dvec3 InVector) {
		return FVector(InVector.x, InVector.y, InVector.z); // glm::dvec3 → FVector 转换（按分量 x,y,z 赋值）
	}

}

// 构造函数：初始化组件和默认属性
ACWeatherSystem::ACWeatherSystem()
{
 	// 禁用 Tick，性能优化 — 天气系统不需要逐帧驱动
	PrimaryActorTick.bCanEverTick = false;
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); // 创建根组件
	RootComponent = RootComp; // 设为 Actor 的根
	SkyAtmosphereComp = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere")); // 创建天空大气渲染组件
	SkyAtmosphereComp->SetupAttachment(RootComp); // 挂载到根组件
	SkyAtmosphereComp->TransformMode = ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform; // 以组件 Transform 位置为地心计算大气
}


// BeginPlay：当游戏开始或被生成时调用
void ACWeatherSystem::BeginPlay()
{
	PWeatherSys::pGCWeatherSystem = this; // 将自身注册为全局单例
	Super::BeginPlay(); // 调用父类 BeginPlay
}

// Tick：逐帧调用（已禁用 bCanEverTick，此函数不会被调用）
void ACWeatherSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UpdateAtmosphereRadius(); // 被注释的大气半径自适应更新
}

// 大世界坐标偏移回调 — Cesium 瓷砖流送时会调用此函数
void ACWeatherSystem::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift); // 父类应用偏移
	P2_ApplyWorldOffset(InOffset, bWorldShift); // 派发到蓝图可覆写事件
}
// 将年月日转换为浮点数（月.日比例），用于天气插值计算
// 例如 2026-06-09 → 6.0 + 9/30.0 = 6.3
float ACWeatherSystem::GetSomedayFloat(int iYear, int iMonth, int iDay)
{
	float fSomedayFloat = 0.0f;
	if (iMonth == 1 || iMonth == 3 || iMonth == 5 || iMonth == 7 || iMonth == 8 || iMonth == 10 || iMonth == 12)
	{
		fSomedayFloat = (iMonth - 1.0) + iDay / 31.0; // 31 天月
	}
	else if (iMonth == 4 || iMonth == 6 || iMonth == 9 || iMonth == 11)
	{
		fSomedayFloat = (iMonth - 1.0) + iDay / 30.0; // 30 天月
	}
	else // 二月
	{
		if ((iYear % 4 == 0 && iYear % 100 != 0) || (iYear % 400 == 0)) // 闰年判断
		{
			fSomedayFloat = (iMonth - 1.0) + iDay / 29.0; // 闰年 29 天
		}
		else
		{
			fSomedayFloat = (iMonth - 1.0) + iDay / 28.0; // 平年 28 天
		}
	}
	return fSomedayFloat; // 返回浮点表示的日期
}

// 根据当前视点高度动态调整大气组件的 BottomRadius
// 使得不同海拔下大气渲染正确（高海拔 = 更大半径 = 更薄大气层）
void ACWeatherSystem::UpdateAtmosphereRadius()
{
	if (!SkyAtmosphereComp || !pGeoRef)return; // 组件或地理参考不存在则退出
	FVector location = UAGlobalUtilBPLibrary::BPGetViewpointTransform().GetTranslation(); // 获取当前视点的世界坐标

	// 使用 CesiumForUnreal 5.x 新 API 将 UE 世界坐标转为经纬高
	FVector LLH_F = pGeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(location); // 返回 (Lon, Lat, Height)
	glm::dvec3 llh = PWSMathConv::UeToGlm(LLH_F); // FVector → glm::dvec3

	// 旧版 API（注释保留）：pGeoRef->TransformUnrealToLongitudeLatitudeHeight(PWSMathConv::UeToGlm(location));

	if (bDebugSkyAtmosphereInfo) // 使用实例自身的天空大气配置
	{
		if (llh.z / 1000.0 > this->SkyAtmosphereInfo.CircumscribedGroundThreshold) { // 高度超过上限阈值
			this->SetSkyAtmosphereGroundRadius(SkyAtmosphereComp, this->SkyAtmosphereInfo.MaxBottomRadius); // 使用最大半径
		}
		else {
			if (llh.z / 1000.0 < this->SkyAtmosphereInfo.InscribedGroundThreshold) { // 高度低于下限阈值
				this->SetSkyAtmosphereGroundRadius(
					SkyAtmosphereComp,
					this->SkyAtmosphereInfo.MinBottomRadius); // 使用最小半径（默认 6365）
			}
			else { // 在阈值范围内，使用线性插值
				double t = // 计算当前高度在两个阈值之间的归一化比例
					((llh.z / 1000.0) - this->SkyAtmosphereInfo.InscribedGroundThreshold) /
					(this->SkyAtmosphereInfo.CircumscribedGroundThreshold - this->SkyAtmosphereInfo.InscribedGroundThreshold);
				double radius = UKismetMathLibrary::MapRangeClamped( // 使用引擎的线性映射函数将高度映射到半径
					llh.z / 1000,
					this->SkyAtmosphereInfo.InscribedGroundThreshold,
					this->SkyAtmosphereInfo.CircumscribedGroundThreshold,
					this->SkyAtmosphereInfo.MinBottomRadius,
					this->SkyAtmosphereInfo.MaxBottomRadius);
				this->SetSkyAtmosphereGroundRadius(SkyAtmosphereComp, radius); // 应用插值半径
			}
		}
	}
	else // 使用全局大气配置（PWeatherSys 命名空间）
	{
		if (llh.z / 1000.0 > PWeatherSys::GSkyAtmosphereInfo.CircumscribedGroundThreshold) { // 超过上限
			this->SetSkyAtmosphereGroundRadius(SkyAtmosphereComp, PWeatherSys::GSkyAtmosphereInfo.MaxBottomRadius); // 最大半径
		}
		else {
			if (llh.z / 1000.0 < PWeatherSys::GSkyAtmosphereInfo.InscribedGroundThreshold) { // 低于下限
				this->SetSkyAtmosphereGroundRadius(
					SkyAtmosphereComp,
					PWeatherSys::GSkyAtmosphereInfo.MinBottomRadius); // 最小半径
			}
			else { // 插值区间
				double t =
					((llh.z / 1000.0) - PWeatherSys::GSkyAtmosphereInfo.InscribedGroundThreshold) /
					(PWeatherSys::GSkyAtmosphereInfo.CircumscribedGroundThreshold - PWeatherSys::GSkyAtmosphereInfo.InscribedGroundThreshold);
				double radius = UKismetMathLibrary::MapRangeClamped(
					llh.z / 1000,
					PWeatherSys::GSkyAtmosphereInfo.InscribedGroundThreshold,
					PWeatherSys::GSkyAtmosphereInfo.CircumscribedGroundThreshold,
					PWeatherSys::GSkyAtmosphereInfo.MinBottomRadius,
					PWeatherSys::GSkyAtmosphereInfo.MaxBottomRadius);
				this->SetSkyAtmosphereGroundRadius(SkyAtmosphereComp, radius); // 应用
			}
		}
	}
}

// 设置天空大气的 BottomRadius，并进行阈值保护
void ACWeatherSystem::SetSkyAtmosphereGroundRadius(USkyAtmosphereComponent* Sky, float Radius)
{
	if (Sky && Radius >= 6300) // 组件有效且半径 ≥ 6300 km（地球半径物理约束）
	{
		Sky->BottomRadius = Radius; // 设置大气地面半径
	}
	if (bEnableDebugInfo)
	{
		UAGlobalUtilBPLibrary::ShowMsgLog(FString::Printf(TEXT("SetSkyAtmosphereGroundRadius: %f"), Radius)); // 调试日志
	}
}
// 获取天气数据的只读引用
const TArray<FWeatherArea>& ACWeatherSystem::GetWeatherData()
{
	return WeatherData; // 返回成员引用
}

// 设置天气数据数组
bool ACWeatherSystem::SetWeatherData(TArray<FWeatherArea>& data)
{
	WeatherData = data; // 浅拷贝替换成员数据
	return true; // 总是成功
}

// 设置天气序列资产的引用路径（预留接口，当前为空实现）
bool ACWeatherSystem::SetWeatherSequenceAssetByRef(FString SequenceAssetPath)
{
	return true; // 空实现，仅返回成功
}
// 将天气数据保存为 JSON 配置文件
// 文件路径: Content/GameCfg/Weather/<InFileName>.json
// JSON 结构: { "Areas": [{ "fOriginLon": ..., "fOriginLat": ..., "fRadius": ..., "Frames": [...] }] }
bool ACWeatherSystem::SaveProfile(FString InFileName)
{
	if (InFileName.IsEmpty())
		InFileName = TEXT("Default.json"); // 默认文件名
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("GameCfg/Weather/") + InFileName); // 拼接完整路径

	TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>(); // JSON 根对象
	TArray<TSharedPtr<FJsonValue>> AreaArray; // 天气区域数组

	for (int i = 0; i < WeatherData.Num(); i++) // 遍历所有天气区域
	{
		TSharedPtr<FJsonObject> AreaObj = MakeShared<FJsonObject>(); // 区域对象
		AreaObj->SetNumberField(TEXT("fOriginLon"), WeatherData[i].fOriginLon); // 区域中心经度
		AreaObj->SetNumberField(TEXT("fOriginLat"), WeatherData[i].fOriginLat); // 区域中心纬度
		AreaObj->SetNumberField(TEXT("fRadius"), WeatherData[i].fRadius); // 区域半径（cm）
		TArray<TSharedPtr<FJsonValue>> FrameArray; // 天气帧数组
		for (int j = 0; j < WeatherData[i].datas.Num(); j++) // 遍历该区域所有天气帧
		{
			TSharedPtr <FJsonObject> tFrame = MakeShared<FJsonObject>(); // 帧对象
			tFrame->SetNumberField(TEXT("fTime"), WeatherData[i].datas[j].fTime); // 时刻
			tFrame->SetNumberField(TEXT("fSunIntensity"), WeatherData[i].datas[j].fSunIntensity); // 太阳强度
			tFrame->SetNumberField(TEXT("fSkyLightIntensity"), WeatherData[i].datas[j].fSkyLightIntensity); // 天光强度
			tFrame->SetNumberField(TEXT("fStarIntensity"), WeatherData[i].datas[j].fStarIntensity); // 星星亮度
			tFrame->SetNumberField(TEXT("fCloudBtm"), WeatherData[i].datas[j].fCloudBtm); // 云底高
			tFrame->SetNumberField(TEXT("fCloudHeight"), WeatherData[i].datas[j].fCloudHeight); // 云高度
			tFrame->SetNumberField(TEXT("fCloudDensity"), WeatherData[i].datas[j].fCloudDensity); // 云密度
			tFrame->SetNumberField(TEXT("fCloudCover"), WeatherData[i].datas[j].fCloudCover); // 云覆盖距离
			tFrame->SetNumberField(TEXT("fCloudAttenuation"), WeatherData[i].datas[j].fCloudAttenuation); // 云减淡距离
			tFrame->SetNumberField(TEXT("fWindIntensity"), WeatherData[i].datas[j].fWindIntensity); // 风力
			tFrame->SetStringField(TEXT("WindDirect"), WeatherData[i].datas[j].WindDirect.ToString()); // 风向（序列化为字符串）
			tFrame->SetNumberField(TEXT("nRainLevel"), WeatherData[i].datas[j].nRainLevel); // 雨级别
			tFrame->SetNumberField(TEXT("nSnowLevel"), WeatherData[i].datas[j].nSnowLevel); // 雪级别
			tFrame->SetNumberField(TEXT("nWetCover"), WeatherData[i].datas[j].nWetCover); // 积水程度
			tFrame->SetNumberField(TEXT("nSnowCover"), WeatherData[i].datas[j].nSnowCover); // 积雪程度
			tFrame->SetNumberField(TEXT("nIceCover"), WeatherData[i].datas[j].nIceCover); // 结冰程度
			tFrame->SetNumberField(TEXT("fVisibility"), WeatherData[i].datas[j].fVisibility); // 能见度
			tFrame->SetNumberField(TEXT("nFogDensity"), WeatherData[i].datas[j].nFogDensity); // 雾浓度
			tFrame->SetNumberField(TEXT("lValidTag"), WeatherData[i].datas[j].lValidTag); // 有效性标识位掩码
			FrameArray.Add(MakeShared<FJsonValueObject>(tFrame)); // 加入帧数组
		}
		AreaObj->SetArrayField(TEXT("Frames"), FrameArray); // 设置区域内的帧数组
		AreaArray.Add(MakeShared<FJsonValueObject>(AreaObj)); // 加入区域数组
	}
	RootObj->SetArrayField(TEXT("Areas"), AreaArray); // 设置根对象的区域数组
	FString JsonStr; // JSON 序列化输出的字符串
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr); // 创建 JSON 写入器
	FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer); // 序列化
	FFileHelper::SaveStringToFile(JsonStr, *filePath); // 写入磁盘文件
	UAGlobalUtilBPLibrary::ShowMsgLog(FString::Printf(TEXT("Save Weather Profile To %s"), *filePath)); // 日志
	return true; // 保存成功
}

// 从 JSON 配置文件加载天气数据
bool ACWeatherSystem::LoadProfile(FString InFileName)
{
	if (InFileName.IsEmpty())
		InFileName = TEXT("Default.json"); // 默认加载 Default.json
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("GameCfg/Weather/") + InFileName); // 拼接路径
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile(); // 获取文件接口
	if (!PlatformFile.FileExists(*filePath)) // 文件不存在
	{
		UAGlobalUtilBPLibrary::ShowMsgLog(FString::Printf(TEXT("File %s Not Exists"), *filePath)); // 错误提示
		return false;
	}
	FString JsonStr; // JSON 文本缓冲区
	FFileHelper::LoadFileToString(JsonStr, *filePath); // 读取文件内容
	if (JsonStr.IsEmpty()) // 空文件
	{
		UAGlobalUtilBPLibrary::ShowMsgLog(FString::Printf(TEXT("File %s Is Empty"), *filePath));
		return false;
	}
	TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>(); // JSON 根对象
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonStr); // JSON 读取器
	FJsonSerializer::Deserialize(JsonReader, RootObj); // 反序列化

	const TArray<TSharedPtr<FJsonValue>> *AreasJsonValue; // 区域数组的 JSON 值指针
	RootObj->TryGetArrayField("Areas", AreasJsonValue); // 从根对象获取 Areas 数组

	WeatherData.Empty(); // 清空现有天气数据

	for (int i = 0; i < AreasJsonValue->Num(); i++) // 遍历每个区域
	{
		FWeatherArea tArea; // 临时区域对象
		tArea.fOriginLon = (*AreasJsonValue)[i]->AsObject()->GetNumberField(TEXT("fOriginLon")); // 经度
		tArea.fOriginLat = (*AreasJsonValue)[i]->AsObject()->GetNumberField(TEXT("fOriginLat")); // 纬度
		tArea.fRadius = (*AreasJsonValue)[i]->AsObject()->GetNumberField(TEXT("fOriginLat")); // 注意：这里实际读取的是 fOriginLat 而非 fRadius — 疑似 bug
		const TArray<TSharedPtr<FJsonValue>>& FramesJsonValue = (*AreasJsonValue)[i]->AsObject()->GetArrayField(TEXT("Frames")); // 获取该区域的帧数组
		for (int j = 0; j < FramesJsonValue.Num(); j++) // 遍历每个帧
		{
			FWeatherFrame tFrame; // 临时帧对象
			tFrame.fTime = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fTime")); // 时刻
			tFrame.fSunIntensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fSunIntensity")); // 太阳强度
			tFrame.fSkyLightIntensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fSkyLightIntensity")); // 天光强度
			tFrame.fStarIntensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fStarIntensity")); // 星星亮度
			tFrame.fCloudBtm = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fCloudBtm")); // 云底高
			tFrame.fCloudHeight = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fCloudHeight")); // 云高度
			tFrame.fCloudDensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fCloudDensity")); // 云密度
			tFrame.fCloudCover = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fCloudCover")); // 云覆盖距离
			tFrame.fCloudAttenuation = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fCloudAttenuation")); // 云减淡距离
			tFrame.fWindIntensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fWindIntensity")); // 风力
			tFrame.WindDirect.InitFromString(FramesJsonValue[j]->AsObject()->GetStringField(TEXT("WindDirect"))); // 风向（从字符串反序列化）
			tFrame.nRainLevel = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nRainLevel")); // 雨级别
			tFrame.nSnowLevel = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nSnowLevel")); // 雪级别
			tFrame.nWetCover = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nWetCover")); // 积水程度
			tFrame.nSnowCover = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nSnowCover")); // 积雪程度
			tFrame.nIceCover = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nIceCover")); // 结冰程度
			tFrame.fVisibility = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("fVisibility")); // 能见度
			tFrame.nFogDensity = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("nFogDensity")); // 雾浓度
			tFrame.lValidTag = FramesJsonValue[j]->AsObject()->GetNumberField(TEXT("lValidTag")); // 有效性标识
			tArea.datas.Add(tFrame); // 添加到区域的帧列表
		}
		WeatherData.Add(tArea); // 添加到天气数据
	}
	UAGlobalUtilBPLibrary::ShowMsgLog(FString::Printf(TEXT("Load Weather Profile %s Success!"), *filePath)); // 成功日志
	return true; // 加载成功
}