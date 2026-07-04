# AWeatherSystem 插件 C++ 代码总结

> 生成日期：2026-06-09  
> 插件路径：`Plugins/AWeatherSystem/Source/AWeatherSystem/Private/`

---

## 文件概览

| 文件 | 主要功能 |
|------|----------|
| `AWeatherSystem.cpp` | UE 模块入口，负责插件的启动与卸载 |
| `CWeatherDataType.cpp` | 定义日志分类，是数据类型头文件的配套实现 |
| `CWeatherSystem.cpp` | 天气系统主 Actor，全局状态管理、大气渲染、JSON 天气配置持久化 |
| `CWeatherBPFLibrary.cpp` | 蓝图函数库，天文算法（日月星位置）、时间计算、Cesium 坐标变换 |

---

## 1. AWeatherSystem.cpp — 模块入口

**对应头文件**: `AWeatherSystem.h`

### 功能说明
这是 AWeatherSystem 插件的**模块入口文件**，实现了 UE 标准的 `IModuleInterface` 接口。

### 关键函数

| 函数 | 说明 |
|------|------|
| `StartupModule()` | 模块加载时调用，当前为空实现（预留初始化扩展点） |
| `ShutdownModule()` | 模块卸载时调用，当前为空实现（预留清理扩展点） |

### 要点
- 通过 `IMPLEMENT_MODULE(FAWeatherSystemModule, AWeatherSystem)` 宏将 `FAWeatherSystemModule` 注册为插件的模块入口
- 定义本地化命名空间 `LOCTEXT_NAMESPACE`

---

## 2. CWeatherDataType.cpp — 数据类型定义

**对应头文件**: `CWeatherDataType.h`

### 功能说明
此文件非常简洁，主要做两件事：
1. **定义日志分类**: `DEFINE_LOG_CATEGORY(LogPWeatherSys)` — 在头文件中通过 `DECLARE_LOG_CATEGORY_EXTERN` 声明，此处进行实现
2. 引入 `CWeatherDataType.h` 头文件，该头文件定义了整个天气系统的**核心数据结构**

### 头文件定义的关键数据结构（上下文参考）

| 结构体/枚举 | 说明 |
|-------------|------|
| `ECloudType` | 云类型枚举：积云(CUMULUS)、絮状云(FLOCCUS)、荚状云(LENTICULARIS)、丝状云(FILWISE) |
| `UE_WEATHER_VALID_TAG` | 天气帧有效性标识枚举（17 种属性，如太阳强度、云密度、雨雪等级等） |
| `FCloudStatInfo` | 云状态信息（云类型） |
| `FWeatherInfo` | 天气状态信息（网络同步时间、闪电队列、模拟时间） |
| `FSkyAtmosphereInfo` | 天空大气配置（高度阈值、最小/最大地形半径） |
| `FWeatherFrame` | 单帧天气数据（17+ 个属性：太阳/天光/星星强度、云参数、风力风向、雨雪等级、能见度、雾浓度等） |
| `FWeatherArea` | 天气区域（经度、纬度、半径、天气帧数组） |
| `FWeatherCurves` | 天气曲线（用于平滑插值的各属性 `FInterpCurveFloat`） |
| `FDateControlData` | 日期控制数据（年、月、日、时区） |
| `FTimeControlData` | 时间控制数据（时间、进度倍率、循环起止、循环开关） |
| `FOnSimTimeUpdate` | 动态多播委托，用于通知模拟时间更新 |
| `UCLoudTypeDataAsset` | 云类型数据资产（3D 噪声贴图配置） |

---

## 3. CWeatherSystem.cpp — 天气系统主 Actor

**对应头文件**: `CWeatherSystem.h`  
**代码行数**: ~303 行  
**依赖**: `CesiumGeoreference`、`CesiumGlobeAnchorComponent`、`SkyAtmosphereComponent`、`Json`、`KismetMathLibrary`

### 功能说明
这是天气系统的**核心 Actor 类**，承担全局单例、大气渲染管理、天气数据 JSON 持久化等职责。

### 全局变量（`PWeatherSys` 命名空间）

| 变量 | 类型 | 说明 |
|------|------|------|
| `pGCWeatherSystem` | `ACWeatherSystem*` | 全局天气系统单例指针 |
| `GSkyAtmosphereInfo` | `FSkyAtmosphereInfo` | 全局天空大气高度阈值与半径配置 |
| `GCloudStatInfo` | `FCloudStatInfo` | 全局云类型状态 |
| `GWeatherInfo` | `FWeatherInfo` | 全局天气信息（网络同步时间、闪电队列、模拟时间） |

### 辅助命名空间 `PWSMathConv`

| 函数 | 说明 |
|------|------|
| `UeToGlm(FVector)` | UE `FVector` → GLM `glm::dvec3` 转换 |
| `GlmToUe(glm::dvec3)` | GLM `glm::dvec3` → UE `FVector` 转换 |

### 关键函数详解

#### 生命周期
| 函数 | 说明 |
|------|------|
| `ACWeatherSystem()` | 构造函数：创建 `RootComp`(USceneComponent) 和 `SkyAtmosphereComp`(USkyAtmosphereComponent)，设置大气变换模式为 `PlanetCenterAtComponentTransform`，**禁用 Tick** |
| `BeginPlay()` | 将 `this` 注册到 `PWeatherSys::pGCWeatherSystem` 全局单例 |
| `Tick(DeltaTime)` | Tick 已被禁用，函数体仅调用 `Super::Tick` |
| `ApplyWorldOffset(InOffset, bWorldShift)` | 大世界坐标偏移回调，适配 Cesium 瓷砖流送的浮点原点重定位，转发到蓝图事件 `P2_ApplyWorldOffset` |

#### 日期计算
| 函数 | 说明 |
|------|------|
| `GetSomedayFloat(iYear, iMonth, iDay)` | 将日期转为浮点数（如 6月9日 → 6.3），**考虑闰年**，各月天数不同（28/29/30/31 天分别处理），用于天气参数的时间插值 |

#### 大气渲染
| 函数 | 说明 |
|------|------|
| `UpdateAtmosphereRadius()` | 根据当前视点高度动态调整 `SkyAtmosphereComponent` 的 `BottomRadius`。通过 Cesium Georeference 将 UE 坐标转为经纬高，在两个高度阈值之间**线性插值**映射半径。支持使用实例自身配置（`bDebugSkyAtmosphereInfo=true`）或全局配置 |
| `SetSkyAtmosphereGroundRadius(Sky, Radius)` | 设置大气底部半径，带 `Radius >= 6300` km 的**物理约束保护**，可选调试日志 |

#### 天气数据管理
| 函数 | 说明 |
|------|------|
| `GetWeatherData()` | 返回 `TArray<FWeatherArea>` 的只读引用 |
| `SetWeatherData(data)` | 替换天气数据数组 |
| `SetWeatherSequenceAssetByRef(Path)` | **预留接口**，当前为空实现 |

#### JSON 持久化
| 函数 | 说明 |
|------|------|
| `SaveProfile(InFileName)` | 将 `WeatherData` 序列化为 JSON 并写入 `Content/GameCfg/Weather/<InFileName>.json`。JSON 结构：`{"Areas": [{"fOriginLon", "fOriginLat", "fRadius", "Frames": [{17+ 个天气属性}]}]}` |
| `LoadProfile(InFileName)` | 从 JSON 文件反序列化加载天气数据到 `WeatherData`。含文件存在性检查和空文件检查。**注意**：`LoadProfile` 中第 305 行读取 `fRadius` 时实际读取了 `fOriginLat` 字段，可能是 bug |

---

## 4. CWeatherBPFLibrary.cpp — 蓝图函数库

**对应头文件**: `CWeatherBPFLibrary.h`  
**代码行数**: ~696 行（本项目最大文件）  
**依赖**: `KismetMathLibrary`、`CWeatherSystem`、`SunPosition.h`

### 功能说明
这是天气系统的**蓝图函数库**，提供全局状态 Getter/Setter、日期计算、天文位置算法（太阳/月亮/星空）、Cesium 坐标变换等功能。几乎所有函数都是 `static`，可直接在蓝图中调用。

### 关键函数详解

#### 全局状态 Getter/Setter（6 个函数）
| 函数 | 说明 |
|------|------|
| `SetNetSyncTime` / `GetNetSyncTime` | 设置/读取 `PWeatherSys::GWeatherInfo.NetSyncTime` |
| `SetGSkyAtmosphereInfo` / `GetGSkyAtmosphereInfo` | 设置/读取全局天空大气配置 |
| `SetGCloudStatInfo` / `GetGCloudStatInfo` | 设置/读取全局云状态 |
| `SetGWeatherInfo` / `GetGWeatherInfo` | 设置/读取全局天气信息 |

#### 日期与时间计算
| 函数 | 说明 |
|------|------|
| `CalcDateAddN(InYear, InMonth, InDay, AddNDay, OutYear, OutMonth, OutDay)` | 输入日期 + N 天后计算新日期。支持**跨月、跨年、闰年**。使用**递归**处理跨年场景，逐月消耗剩余天数 |
| `PGetHMSFromSolarTime(SolarTime, Hour, Minute, Second)` | 将太阳时（小数小时，如 6.5）分解为时/分/秒 |
| `CheckUEWeatherValidTag(InVal, InCheckTag)` | 按位检查天气帧有效性标识中是否置位了指定标记 |
| `CalcSolarTimeRot(SolarTime, Latitude, Longitude, TimeZone, Year, Month, Day)` | 根据太阳时和地理坐标计算太阳在场景中的旋转（Pitch=高度角，Yaw=方位角-90°） |

#### 天文算法辅助函数（内部使用，基于 JP Meeus 天文算法）

| 函数 | 说明 |
|------|------|
| `getJD(Day, Month, Year)` | **儒略日计算**：格里高利历日期 → 连续儒略日数（标准公式，含世纪闰年修正） |
| `getGAST(GST0, UT)` | **格林尼治视恒星时**：`GAST = GST0 + 1.00273790935 × UT`，归一化到 [0, 360) |
| `getGST0(Seconds, Minutes, Hours, Day, Month, Year)` | **世界时 0h 恒星时**：使用 IAU 2000 公式的三次多项式计算 |
| `getHourAngle(GST0, UT, RightAscension, Longitude)` | **地方时角**：`LHA = GAST + Longitude - RightAscension` |
| `getElevation(Declination, HourAngle, Latitude)` | **高度角**：通过球面三角函数 `asin(sin(δ)sin(φ) + cos(δ)cos(HA)cos(φ))` 计算 |
| `getAzimuth(Declination, HourAngle, Latitude)` | **方位角**：通过 `atan2` 计算，结果 +180° 转为天文学方位角（0=北） |
| `getRotationAngle(Declination, RightAscension, GST0, UT, Longitude, Latitude)` | **视场旋转角**：用于 skybox 中星体贴图的旋转校正 |

##### 月球轨道周期项求和（ELP-2000/82 简化模型）

| 函数 | 项数 | 说明 |
|------|------|------|
| `getSumOfPeriodicTermsOfLongitude(D, M, M1, F, E, A1, A2, L1)` | 60 项 | 月球**视经度**周期项求和（含金星摄动项）。系数范围从 -185116 到 +6288774（单位：角秒/10000） |
| `getSumOfPeriodicTermsOfLatitude(D, M, M1, F, E, A1, A3, L1)` | 60 项 | 月球**视黄纬**周期项求和。系数范围从 -2235 到 +5128122 |
| `getSumOfPeriodicTermsOfDistance(D, M, M1, F, E)` | 46 项 | 月球**地月距离**周期项求和。系数范围从 -20905355 到 +246158 |

#### 日月星位置计算（Sky Position Plugin 系列）

| 函数 | 说明 |
|------|------|
| `GetMoonPositionSPP(Latitude, Longitude, TimeZone, NorthOffset, bIsDaylightSavingTime, Year, Month, Day, Hours, Minutes, Seconds, MoonData)` | **月球位置计算**（~120 行，最复杂函数）。流程：①本地时间→UTC ②计算 8 个平轨道参数（L1, D, M, M1, F, A1, A2, A3）③调用三项周期求和函数 ④计算章动和真黄赤交角 ⑤计算视差修正 ⑥赤经赤纬转换 ⑦高度角/方位角/旋转角计算 ⑧天平动计算 ⑨月相照明率计算。输出到 `FMMoonData` 结构体（含高度角、方位角、距离、天平动、月相、赤经赤纬等 11 个字段） |
| `GetSunPositionSPP(...)` | **太阳位置计算**（简化版）：调用 UE5 内置 `USunPositionFunctionLibrary::GetSunPosition`，输出修正高度角和方位角（-90° 将正南转为正东基准） |
| `GetStarsPositionSPP(...)` | **星空位置计算**：①本地时间→UTC ②计算 GST0 ③以春分点（赤经=0, 赤纬=0）为参考，计算其地方时角、高度角、方位角 ④计算星空球旋转（Pitch=高度角, Yaw=方位角+指北偏移, Roll=视场旋转角）。输出 `FRotator` |
| `CalculateMoonRotationSPP(LibrationInLatitude, LibrationInLongitude, PositionAngle, RotationAngle, MoonRotator)` | **月面纹理旋转**：根据天平动和位置角计算月面贴图的三轴旋转（Pitch/Yaw/Roll），用于在 skybox 中正确显示月球表面朝向 |

#### 工具函数

| 函数 | 说明 |
|------|------|
| `CalcDeltaAngle(InAngle, NormalUp, NormalForward)` | 计算向量与上方向的**带符号夹角**：用叉积判断方向，左侧为负时翻转角度 |
| `TransformRotationENUtoUe(GeoReference, UeLoc, EnuRot)` | **ENU→UE 旋转变换**：将 ENU（东北天）坐标系的 Rotator 转换到 UE 世界空间。先 Yaw-90° 对齐基准方向，再通过 Cesium 的 `ComputeEastSouthUpToUnrealTransformation` 计算四元数变换 |
| `TransformLonLatHeightToUe(GeoReference, Lon, Lat, Height)` | **经纬高→UE 坐标**：将字符串格式的经纬高转换为 UE 世界坐标，调用 Cesium 的 `TransformLongitudeLatitudeHeightPositionToUnreal` |
| `GetAbsUeToEcefMatrix(InGeoRef)` | **UE→ECEF 变换矩阵**：获取 UE 绝对世界到地心固连坐标系 (ECEF) 的 4×4 变换矩阵。注意 GLM 列主序 → UE 行主序的**行列转置** |

---

## 整体架构总结

```
AWeatherSystem 插件
├── 模块层 (AWeatherSystem.cpp)
│   └── FAWeatherSystemModule: 插件启动/卸载
│
├── 数据层 (CWeatherDataType.h + CWeatherDataType.cpp)
│   ├── 枚举: ECloudType, UE_WEATHER_VALID_TAG
│   ├── 结构体: FWeatherFrame, FWeatherArea, FWeatherCurves, FWeatherInfo, 
│   │          FSkyAtmosphereInfo, FCloudStatInfo, FDateControlData, FTimeControlData
│   └── 日志: LogPWeatherSys
│
├── 核心层 (CWeatherSystem.h + CWeatherSystem.cpp)
│   ├── ACWeatherSystem Actor: 全局单例
│   ├── 大气渲染管理: UpdateAtmosphereRadius / SetSkyAtmosphereGroundRadius
│   ├── JSON 持久化: SaveProfile / LoadProfile
│   └── 全局状态: PWeatherSys 命名空间
│
└── 工具层 (CWeatherBPFLibrary.h + CWeatherBPFLibrary.cpp)
    ├── 全局状态 Getter/Setter (6 个)
    ├── 日期计算: CalcDateAddN, GetSomedayFloat
    ├── 天文算法: getJD, getGAST, getGST0, getHourAngle, getElevation, getAzimuth
    ├── 月球位置 (60+60+46 项周期展开): GetMoonPositionSPP
    ├── 太阳位置: GetSunPositionSPP
    ├── 星空位置: GetStarsPositionSPP
    ├── 月面旋转: CalculateMoonRotationSPP
    └── Cesium 坐标变换: TransformRotationENUtoUe, TransformLonLatHeightToUe, GetAbsUeToEcefMatrix
```

### 关键依赖

- **Cesium for Unreal**: 地理参考 (Georeference)、坐标变换（经纬高 ↔ UE 坐标、ENU 变换、ECEF 矩阵）
- **UE5 SkyAtmosphere**: 天空大气渲染组件
- **UE5 SunPosition**: 内置太阳位置计算库
- **GLM**: 双精度向量/矩阵数学（通过 Cesium 间接使用）
- **UE JSON**: 天气配置文件的序列化/反序列化
- **AGlobalUtil**: 项目自定义工具库（日志记录、视点查询）

### 潜在问题标记

1. **`CWeatherSystem.cpp:305`** — `LoadProfile` 中读取 `fRadius` 时实际使用的是 `GetNumberField("fOriginLat")`，疑似复制粘贴错误，应改为 `"fRadius"`
2. **`CWeatherSystem.cpp:57`** — `Tick` 已通过 `bCanEverTick = false` 禁用，函数体内注释掉的 `UpdateAtmosphereRadius()` 调用无法执行；大气半径更新需要外部主动调用
3. **`CWeatherBPFLibrary.cpp:559`** — `GetSunPositionSPP` 中注释指出 `USunPositionFunctionLibrary::GetSunPosition` 已被 UE 标记为"废弃但可用"
