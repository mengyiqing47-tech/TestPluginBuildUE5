# UE 项目 C++ 代码分析文档

本目录包含对 UE5 项目 **TestPluginBuildUE5** 中自定义 C++ 源代码（不包含第三方库 CesiumForUnreal）的逐类分析文档。

## 项目结构概览

---

## 1. 主项目模块：TestPluginBuildUE5
- **位置**: `Source/TestPluginBuildUE5/`
- **角色**: 游戏主模块，仅包含模块入口，无自定义类
- [查看详细分析 →](Docs/CodeAnalysis/01-MainModule.md)

---

## 2. 插件 AGlobalUtil
- **位置**: `Plugins/AGlobalUtil/`
- **角色**: 全局工具库，提供 INI 配置读写、图片处理、文件监控、位操作、跨平台进程锁与共享内存
- 包含 **4 个类**、**3 个枚举**、**5 个结构体**、**4 个全局宏**
- [查看详细分析 →](Docs/CodeAnalysis/02-AGlobalUtil.md)

---

## 3. 插件 AWeatherSystem
- **位置**: `Plugins/AWeatherSystem/`
- **角色**: 天气系统，管理日月星辰位置、时间控制、大气参数、云类型、雨雪雾等天气效果
- 包含 **4 个类**、**2 个枚举**、**13 个结构体**、**1 个委托**
- [查看详细分析 →](Docs/CodeAnalysis/03-AWeatherSystem.md)

---

## 类/结构体清单速查

| 模块 | 类型 | 名称 | 说明 |
|------|------|------|------|
| **Main** | Module | `FDefaultGameModuleImpl` | 游戏主模块入口 |
| **AGlobalUtil** | Module | `FAGlobalUtilModule` | 插件模块入口 |
| **AGlobalUtil** | Class | `UGlobalConfigBPLibrary` | INI 配置文件读写函数库 |
| **AGlobalUtil** | Class | `UAGlobalUtilBPLibrary` | 通用工具函数库（图片/文件/位操作/曲线） |
| **AGlobalUtil** | Class | `UtilProcessLock` | 跨平台进程锁 |
| **AGlobalUtil** | Class | `UtilShareMemory` | 跨平台共享内存 |
| **AGlobalUtil** | Enum | `EPensorImageFormat` | 图片格式枚举 |
| **AGlobalUtil** | Enum | `EPFileChangeAction` | 文件变更动作枚举 |
| **AGlobalUtil** | Enum | `EUtilRGBFormat` | 像素格式枚举 |
| **AWeatherSystem** | Module | `FAWeatherSystemModule` | 插件模块入口 |
| **AWeatherSystem** | Class | `ACWeatherSystem` | 天气系统主 Actor |
| **AWeatherSystem** | Class | `UCWeatherBPFLibrary` | 天气函数库（日月位置/坐标转换） |
| **AWeatherSystem** | Class | `UCLoudTypeDataAsset` | 云类型数据资产 |
| **AWeatherSystem** | Enum | `ECloudType` | 云类型枚举 |
| **AWeatherSystem** | Enum | `UE_WEATHER_VALID_TAG` | 天气帧有效标识 |
