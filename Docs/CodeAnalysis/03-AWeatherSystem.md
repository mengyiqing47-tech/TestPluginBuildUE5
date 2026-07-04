# 插件 AWeatherSystem 代码分析

## 文件清单

| 角色 | 路径 |
|------|------|
| 模块入口 | [Private/AWeatherSystem.cpp](../Plugins/AWeatherSystem/Source/AWeatherSystem/Private/AWeatherSystem.cpp) |
| 模块头 | [Public/AWeatherSystem.h](../Plugins/AWeatherSystem/Source/AWeatherSystem/Public/AWeatherSystem.h) |
| 天气系统主 Actor 头 | [Public/CWeatherSystem.h](../Plugins/AWeatherSystem/Source/AWeatherSystem/Public/CWeatherSystem.h) |
| 天气系统主 Actor 实现 | [Private/CWeatherSystem.cpp](../Plugins/AWeatherSystem/Source/AWeatherSystem/Private/CWeatherSystem.cpp) |
| 数据类型 | [Public/CWeatherDataType.h](../Plugins/AWeatherSystem/Source/AWeatherSystem/Public/CWeatherDataType.h) |
| 数据类型实现 | [Private/CWeatherDataType.cpp](../Plugins/AWeatherSystem/Source/AWeatherSystem/Private/CWeatherDataType.cpp) |
| 天气函数库 | [Public/CWeatherBPFLibrary.h](../Plugins/AWeatherSystem/Source/AWeatherSystem/Public/CWeatherBPFLibrary.h) |
| 天气函数库实现 | [Private/CWeatherBPFLibrary.cpp](../Plugins/AWeatherSystem/Source/AWeatherSystem/Private/CWeatherBPFLibrary.cpp) |

---

## 模块类

### FAWeatherSystemModule

| 属性 | 说明 |
|------|------|
| **基类** | `IModuleInterface` |
| **分类** | UE 插件模块类 |
| **API 导出** | `AWEATHERSYSTEM_API` |

#### 职责范围

标准 UE 插件模块入口。`StartupModule()` 和 `ShutdownModule()` 当前均为空实现。插件功能通过 UCLASS 反射自动注册。

---

## 全局变量命名空间

定义在 `CWeatherSystem.h` 头文件中，实现在 `CWeatherSystem.cpp` 中。

```cpp
namespace PWeatherSys {
    ACWeatherSystem* pGCWeatherSystem = nullptr;     // 全局天气系统单例指针
    FSkyAtmosphereInfo GSkyAtmosphereInfo;             // 天空大气信息
    FCloudStatInfo GCloudStatInfo;                     // 云状态信息
    FWeatherInfo GWeatherInfo;                         // 天气状态信息
}
```

在 `BeginPlay()` 时，`ACWeatherSystem` 将自身注册到 `pGCWeatherSystem`：

```cpp
PWeatherSys::pGCWeatherSystem = this;
```

同时，`CWeatherBPFLibrary.h` 提供了一组 Getter/Setter 蓝图函数来读写这些全局状态。这是一种**全局单例 + 蓝图函数库封装**的模式。

---

## 天气系统主 Actor

### ACWeatherSystem

| 属性 | 说明 |
|------|------|
| **基类** | `AActor` |
| **API 导出** | `AWEATHERSYSTEM_API` |
| **Tick** | 默认关闭（`bCanEverTick = false`） |
| **所在文件** | `CWeatherSystem.h/.cpp` |

#### 组件结构

构造时默认创建：

1. **根组件** (`RootComp`) —— `USceneComponent`
2. **天空大气组件** (`SkyAtmosphereComp`) —— `USkyAtmosphereComponent`
   - 挂载到根组件下
   - 变换模式：`PlanetCenterAtComponentTransform`

#### 核心属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `WeatherData` | `TArray<FWeatherArea>` | 天气区域数据集合（可编辑） |
| `WeatherCurves` | `TArray<FWeatherCurves>` | 所有区域的插值天气曲线 |
| `CurFrame` | `FWeatherCurveFrame` | 当前插值帧数据，用于 UpdateWeather |
| `LightCountBuffer` | `TArray<uint8>` | 闪电计数缓冲区 |
| `SkyAtmosphereInfo` | `FSkyAtmosphereInfo` | 调试用大气信息（条件编辑） |
| `SkyAtmosphereComp` | `USkyAtmosphereComponent*` | 天空大气组件引用 |
| `RootComp` | `USceneComponent*` | 根组件 |
| `pGeoRef` | `ACesiumGeoreference*` | Cesium 地理参考引用 |
| `OnSimTimeUpdate` | `FOnSimTimeUpdate` | 模拟时间更新事件 |
| `bEnableDebugInfo` | `bool` | 是否启用调试信息 |

---

#### 蓝图可覆写事件（BlueprintImplementableEvent）

| 事件 | 类别 | 参数 | 说明 |
|------|------|------|------|
| `P2_SetTime` | 时间 | `FTimeControlData` | 设置当前时间（时/分/秒 + 循环） |
| `P2_SetDate` | 时间 | `FDateControlData` | 设置当前日期（年/月/日/时区） |
| `P2_SetTimeSection` | 时间 | `uint8` | 设置时段：0黎明 / 1白天 / 2傍晚 / 3黑夜 |
| `P2_SetWeatherAreaData` | 天气 | `TArray<FWeatherArea>&` | 设置天气区域数据 |
| `P2_ApplyWorldOffset` | 天气 | `FVector, bool` | 大世界偏移回调 |
| `SetCloudType` | 天气 | `ECloudType` | 设置云类型 |
| `SetSeaStatLevel` | 天气 | `uint8` | 设置海面状态等级 |
| `SetMoonLightIntensity` | 天气 | `float` | 设置月光强度 |
| `LinkSensor` | 传感器 | — | 绑定传感器委托 |
| `SetStarParams` | 天气 | `float*4` | 设置星星亮度参数 |
| `ReCaptureSkyLight` | 天气 | — | 重新捕获天光 |

---

#### C++ 实现函数

| 函数 | 说明 |
|------|------|
| `GetSomedayFloat(int year, month, day)` | 将年月日转换为浮点数（月.日，用于插值计算） |
| `UpdateAtmosphereRadius()` | 根据视点高度动态调整大气半径 |
| `SetSkyAtmosphereGroundRadius(Sky, Radius)` | 设置天空大气地面半径（有阈值保护） |
| `GetSensorConfig(Section, Path)` | 读取传感器组件开关配置，返回按位控制值 |
| `GetWeatherData()` | 返回天气数据只读引用 |
| `SetWeatherData(TArray<FWeatherArea>&)` | 设置天气数据 |
| `SaveProfile(FString)` | 将天气数据保存为 JSON 配置文件 |
| `LoadProfile(FString)` | 从 JSON 配置文件加载天气数据 |
| `SetWeatherSequenceAssetByRef(FString)` | 设置天气序列资源引用（当前为空实现） |

---

#### 大气半径自适应逻辑（`UpdateAtmosphereRadius`）

根据视点高度（海拔）动态调整 `USkyAtmosphereComponent` 的 `BottomRadius`，使得在不同海拔下天空大气渲染正确。

**算法**：
1. 通过 `CesiumGeoreference` 将 UE 世界坐标转换为经纬高（LLH）
2. 根据高度（单位 Km）与阈值比较：
   - 高度 > `CircumscribedGroundThreshold`（默认 100 Km）→ 设为 `MaxBottomRadius`（6387 Km）
   - 高度 < `InscribedGroundThreshold`（默认 -20 Km）→ 设为 `MinBottomRadius`（6365 Km）
   - 中间值 → 线性插值 `MapRangeClamped`
3. 支持 `bDebugSkyAtmosphereInfo` 切换使用实例信息或全局信息

---

#### 配置文件序列化（JSON）

**SaveProfile** 将 `WeatherData` 序列化为嵌套 JSON：

```
{
  "Areas": [
    {
      "fOriginLon": ...,
      "fOriginLat": ...,
      "fRadius": ...,
      "Frames": [
        { "fTime": ..., "fSunIntensity": ..., ... },
        ...
      ]
    }
  ]
}
```

**LoadProfile** 反向解析，保存路径为 `Content/GameCfg/Weather/<FileName>`。

---

#### 传感器配置读取（`GetSensorConfig`）

从 INI 配置文件读取 8 个传感器相关组件的开关状态，使用位掩码（`Msetbit` / `Mclrbit`）编码为 `uint8`：

| 位 | 组件 |
|----|------|
| 0 | SkyAtmosphere |
| 1 | SunLight |
| 2 | Moon |
| 3 | VolumetricCloud |
| 4 | RainAndSnow |
| 5 | SkyLight |
| 6 | MoonLight |
| 7 | WeatherPP |

---

## 蓝图函数库

### UCWeatherBPFLibrary

| 属性 | 说明 |
|------|------|
| **基类** | `UBlueprintFunctionLibrary` |
| **API 导出** | `AWEATHERSYSTEM_API` |
| **所在文件** | `CWeatherBPFLibrary.h/.cpp` |

#### 功能组分析

##### 1. 全局状态 Getter/Setter

对 `PWeatherSys::GWeatherInfo`、`GSkyAtmosphereInfo`、`GCloudStatInfo` 的存取封装：

| Getter | Setter |
|--------|--------|
| `GetNetSyncTime` | `SetNetSyncTime` |
| `GetGSkyAtmosphereInfo` | `SetGSkyAtmosphereInfo` |
| `GetGCloudStatInfo` | `SetGCloudStatInfo` |
| `GetGWeatherInfo` | `SetGWeatherInfo` |

---

##### 2. 日期计算

| 函数 | 说明 |
|------|------|
| `CalcDateAddN(Year, Month, Day, AddNDay, OutYear, OutMonth, OutDay)` | 输入日期 + N 天后计算新的年/月/日 |

**算法**：通过月份天数数组计算一年中的第几天，递归处理跨年。

---

##### 3. 天气标识检查

| 函数 | 说明 |
|------|------|
| `CheckUEWeatherValidTag(InVal, InCheckTag)` | 检查 `lValidTag` 中指定标识位是否置位 |

底层：`InVal & (1 << (int32)InCheckTag)`

---

##### 4. 时间转换

| 函数 | 说明 |
|------|------|
| `PGetHMSFromSolarTime(SolarTime, Hour, Minute, Second)` | 将太阳时（浮点小时数）转换为时/分/秒 |

---

##### 5. 天文位置计算

使用 JPL/天文算法 + SunPosition 库计算日月位置：

| 函数 | 输入 | 输出 |
|------|------|------|
| `GetMoonPositionSPP(Lat, Lon, TimeZone, NorthOffset, 夏令时, Y/M/D, H/M/S)` | 地理/时间参数 | `FMMoonData`（10 项月球天文数据） |
| `GetSunPositionSPP(同上)` | 同上 | `FMSunData`（5 项太阳天文数据） |
| `GetStarsPositionSPP(同上)` | 同上 | `FRotator`（星空球旋转） |
| `CalculateMoonRotationSPP(经/纬度libration, 位置角, 旋转角)` | 月面参数 | `FRotator`（月面旋转） |

**实现细节**：`GetMoonPositionSPP` 和 `GetStarsPositionSPP` 使用了完整的**月球/恒星天文算法**，包含：
- 儒略日（JD）计算
- 月球经度/纬度/距离的周期项求和（数十项三角级数展开）
- 黄赤交角、视差、章动修正
- GMST（格林尼治恒星时）计算
- 高度角/方位角转换

这些算法源自天文历书（Astronomical Almanac）的标准公式。

---

##### 6. 坐标转换

| 函数 | 说明 |
|------|------|
| `CalcSolarTimeRot(SolarTime, Lat, Lon, TimeZone, Y/M/D)` | 根据太阳时计算太阳旋转（俯仰/偏航），使用 `USunPositionFunctionLibrary` |
| `TransformRotationENUtoUe(GeoRef, UE位置, ENU旋转)` | 将 ENU 坐标系旋转转换为 UE 坐标系旋转（减 90° Yaw 偏置 + 四元数变换） |
| `TransformLonLatHeightToUe(GeoRef, Lon字符串, Lat字符串, Height字符串)` | 经/纬/高字符串转换为 UE 世界坐标（`CesiumGeoreference::TransformLongitudeLatitudeHeightPositionToUnreal`） |
| `GetAbsUeToEcefMatrix(GeoRef)` | 获取 UE 世界到 ECEF 地心固联坐标系的 4x4 变换矩阵 |

---

##### 7. 角度计算

| 函数 | 说明 |
|------|------|
| `CalcDeltaAngle(InAngle, NormalUp, NormalForward)` | 计算向量与上方向的夹角，带方向符号（叉积判断左右） |

---

## 数据资产类

### UCLoudTypeDataAsset

| 属性 | 说明 |
|------|------|
| **基类** | `UDataAsset` |
| **BlueprintType** | 是 |
| **分类** | 云类型配置数据 |

#### 属性

| 字段 | 类型 | 说明 |
|------|------|------|
| `CloudNoiseTile3D` | `FLinearColor` | 3D 云噪声贴图的平铺参数（X/Y/Z） |
| `CloudNoiseTile3DTop` | `FLinearColor` | 云层顶部噪声贴图平铺参数 |

通过 `FLinearColor` 的四个通道（RGBA）分别编码不同维度的平铺频率。

---

## 枚举定义

### ECloudType

| 值 | 中文 | 说明 |
|----|------|------|
| `CUMULUS` | 积云 | 蓬松的棉花状云朵 |
| `FLOCCUS` | 絮状云 | 破碎絮状云 |
| `LENTICULARIS` | 荚状云 | 透镜状云（常出现在山脊附近） |
| `FILWISE` | 丝状云 | 纤维丝状云 |

---

### UE_WEATHER_VALID_TAG

天气帧有效标识枚举，共 18 个标识位，用于 `FWeatherFrame.lValidTag` 的位掩码：

| 枚举值 | 对应字段 | 说明 |
|--------|---------|------|
| `ESUN_INT` | `fSunIntensity` | 太阳强度有效 |
| `ESKY_LIGHT_INT` | `fSkyLightIntensity` | 天光强度有效 |
| `ESTAR_INT` | `fStarIntensity` | 星星亮度有效 |
| `ECLOUD_BTM` | `fCloudBtm` | 云底高有效 |
| `ECLOUD_HEIGHT` | `fCloudHeight` | 云高度有效 |
| `ECLOUD_DENSITY` | `fCloudDensity` | 云密度有效 |
| `ECLOUD_COVER` | `fCloudCover` | 云覆盖距离有效 |
| `ECLOUD_ATTENUATION` | `fCloudAttenuation` | 云减淡距离有效 |
| `EWIND_INT` | `fWindIntensity` | 风力有效 |
| `EWIND_DIRECT` | `WindDirect` | 风向有效 |
| `ERAIN_LEVEL` | `nRainLevel` | 雨级别有效 |
| `ESNOW_LEVEL` | `nSnowLevel` | 雪级别有效 |
| `EWET_COVER` | `nWetCover` | 积水程度有效 |
| `ESNOW_COVER` | `nSnowCover` | 积雪程度有效 |
| `EICE_COVER` | `nIceCover` | 结冰程度有效 |
| `EVISIBILITY` | `fVisibility` | 能见度有效 |
| `EFOGDENSITY` | `nFogDensity` | 雾浓度有效 |
| `ESKYFOG` | `nSkyFog` | 天空雾有效 |

---

## 结构体定义

### FCloudStatInfo（USTRUCT）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Type` | `ECloudType` | CUMULUS | 云类型 |

---

### FWeatherInfo（USTRUCT）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `NetSyncTime` | `int32` | 0 | 网络同步时间戳 |
| `LightningArray` | `TArray<int32>` | 空 | 闪电队列（0 表示无闪电） |
| `SimTime` | `float` | 0.0 | 模拟时间 |

---

### FSkyAtmosphereInfo（USTRUCT）

大气自适应半径配置：

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `InscribedGroundThreshold` | `float` | -20 | 开始调整大气的海拔阈值（Km） |
| `CircumscribedGroundThreshold` | `float` | 100 | 结束调整大气的海拔阈值（Km） |
| `MinBottomRadius` | `float` | 6365 | 最小大气地面半径（Km） |
| `MaxBottomRadius` | `float` | 6387 | 最大大气地面半径（Km） |

---

### FWeatherFrame（USTRUCT）

天气帧的离散采样数据，共 16 个环境参数 + 1 个位有效性标识：

| 字段 | 类型 | 说明 |
|------|------|------|
| `fTime` | `float` | 时刻（太阳时，0-24） |
| `fSunIntensity` | `float` | 太阳强度 |
| `fSkyLightIntensity` | `float` | 天光强度 |
| `fStarIntensity` | `float` | 星星亮度 |
| `fCloudBtm` | `float` | 云底高 |
| `fCloudHeight` | `float` | 云高度 |
| `fCloudDensity` | `float` | 云密度 |
| `fCloudCover` | `float` | 云覆盖距离（Km） |
| `fCloudAttenuation` | `float` | 云减淡距离（Km） |
| `fWindIntensity` | `float` | 风力 |
| `WindDirect` | `FVector` | 风向 |
| `nRainLevel` | `uint8` | 雨级别 |
| `nSnowLevel` | `uint8` | 雪级别 |
| `nWetCover` | `uint8` | 积水程度 |
| `nSnowCover` | `uint8` | 积雪程度 |
| `nIceCover` | `uint8` | 结冰程度 |
| `fVisibility` | `float` | 能见度 |
| `nFogDensity` | `uint8` | 雾浓度 |
| `nSkyFog` | `uint8` | 天空雾 |
| `lValidTag` | `int32` | 有效性位标识（参照 `UE_WEATHER_VALID_TAG`） |

---

### FWeatherCurveFrame（USTRUCT）

插值后的天气帧，与 `FWeatherFrame` 的区别在于雨/雪/积水/积雪/结冰/雾/天空雾使用 `float` 而非 `uint8`（插值结果可能是小数）：

| 字段 | 类型 | 说明 |
|------|------|------|
| 前 11 项 | 同 `FWeatherFrame` | 同 |
| `nRainLevel~fSkyFog` | `float` | 雨雪雾等参数（浮点数插值版本） |

---

### FWeatherArea（USTRUCT）

天气区域数据：

| 字段 | 类型 | 说明 |
|------|------|------|
| `fOriginLon` | `float` | 区域中心经度 |
| `fOriginLat` | `float` | 区域中心纬度 |
| `fRadius` | `float` | 区域半径（cm） |
| `datas` | `TArray<FWeatherFrame>` | 该区域的所有天气帧 |

---

### FWeatherCurves（USTRUCT）

天气插值曲线集合，每个环境参数对应一条 `FInterpCurveFloat`（风向使用 `FInterpCurveVector`）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `OriginLon` / `OriginLat` / `Radius` | `float` | 区域地理信息 |
| `SunIntensity` ~ `SkyFog` | `FInterpCurveFloat` | 16 条浮点插值曲线 |
| `WindDirect` | `FInterpCurveVector` | 风向矢量插值曲线 |

共计 **18 条插值曲线**（17 Float + 1 Vector）。

---

### FTimeControlData（USTRUCT）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `fTime` | `float` | 0 | 当前时间（太阳时） |
| `fTimeProgressionScale` | `float` | 0 | 时间流逝速度倍率 |
| `fTimeLoopStart` | `float` | 0 | 时间循环开始值 |
| `fTimeLoopEnd` | `float` | 0 | 时间循环结束值 |
| `ValueTag` | `uint8` | 0 | 有效性标识（位控制：循环/进度/时间） |
| `bTimeLoop` | `bool` | false | 是否开启时间循环 |

---

### FDateControlData（USTRUCT）

| 字段 | 类型 | 说明 |
|------|------|------|
| `Year` | `int32` | 年 |
| `Month` | `uint8` | 月 |
| `Day` | `uint8` | 日 |
| `TimeZone` | `float` | 时区（如 8.0 表示东八区） |

---

### FMMoonData（USTRUCT）

月球天文位置数据，共 10 个字段：

| 字段 | 类型 | 说明 |
|------|------|------|
| `Elevation` | `float` | 高度角（度） |
| `Azimuth` | `float` | 方位角（度） |
| `RotationAngle` | `float` | 自转角度（度） |
| `DistanceToTheMoon` | `float` | 地月距离 |
| `LibrationInLatitude` | `float` | 纬度天平动 |
| `LibrationInLongitude` | `float` | 经度天平动 |
| `PositionAngle` | `float` | 位置角 |
| `DiskIllumination` | `float` | 月面照亮百分比 |
| `Declination` | `float` | 赤纬 |
| `RightAscension` | `float` | 赤经（时角） |

---

### FMSunData（USTRUCT）

太阳天文位置数据：

| 字段 | 类型 | 说明 |
|------|------|------|
| `Elevation` | `float` | 高度角（度） |
| `Azimuth` | `float` | 方位角（度） |
| `Distance` | `float` | 距离 |
| `Declination` | `float` | 赤纬 |
| `RightAscension` | `float` | 赤经（时角） |

---

## 委托定义

### FOnSimTimeUpdate

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimTimeUpdate, float, SimTime);
```

模拟时间更新时的多播委托，用于通知其他系统当前天气模拟时间的变化。

---

## 工具命名空间

在 `CWeatherSystem.cpp` 中定义：

```cpp
namespace PWSMathConv {
    glm::dvec3 UeToGlm(FVector);    // UE FVector → glm::dvec3
    FVector GlmToUe(glm::dvec3);    // glm::dvec3 → UE FVector
}
```

用于 UE 向量类型与 GLM 数学库之间的基础转换，在与 CesiumForUnreal 交互时需要用到。

---

## 日志分类

在 `CWeatherDataType.h` 中声明，在 `CWeatherDataType.cpp` 中定义：

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogPWeatherSys, Log, All);
DEFINE_LOG_CATEGORY(LogPWeatherSys);
```

天气系统专用的日志分类 `LogPWeatherSys`，日志级别 `Log`，所有消息均显示。

---

## 依赖关系

```
AWeatherSystem
 ├── Core / Engine（UE 基础）
 ├── CesiumForUnreal（CesiumGeoreference / CesiumGlobeAnchorComponent）
 ├── SunPosition（天文位置库）
 ├── Json（UE JSON 序列化）
 └── AGlobalUtil（UAGlobalUtilBPLibrary / UGlobalConfigBPLibrary / 位操作宏）
```
