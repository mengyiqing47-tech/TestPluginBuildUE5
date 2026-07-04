// Copyright 2020-2024 CesiumGS, Inc. and Contributors
// CesiumRuntime 模块入口 — 负责模块生命周期管理、全局异步系统、资产访问器、缓存数据库的初始化

#include "CesiumRuntime.h" // 模块头文件
#include "Cesium3DTilesContent/registerAllTileContentTypes.h" // 注册所有瓦片内容类型
#include "CesiumAsync/CachingAssetAccessor.h" // 带缓存的资产访问器
#include "CesiumAsync/GunzipAssetAccessor.h" // Gzip 解压资产访问器
#include "CesiumAsync/SqliteCache.h" // SQLite 缓存后端
#include "CesiumRuntimeSettings.h" // 运行时设置（缓存数量等）
#include "CesiumUtility/Tracing.h" // CESIUM_TRACE 性能追踪宏
#include "HAL/FileManager.h" // 文件管理接口
#include "HttpModule.h" // HTTP 模块
#include "Interfaces/IPluginManager.h" // 插件管理器
#include "Misc/Paths.h" // 路径工具
#include "ShaderCore.h" // 着色器资源映射
#include "SpdlogUnrealLoggerSink.h" // spdlog → UE 日志桥接
#include "UnrealAssetAccessor.h" // UE 平台的资产访问器实现
#include "UnrealTaskProcessor.h" // UE 平台的异步任务处理器
#include <CesiumAsync/AsyncSystem.h> // Cesium 异步系统
#include <CesiumAsync/IAssetAccessor.h> // 资产访问器接口
#include <Modules/ModuleManager.h> // 模块管理器
#include <spdlog/spdlog.h> // spdlog 日志库

#if CESIUM_TRACING_ENABLED
#include <chrono> // 性能追踪需要 chrono 时间戳
#endif

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule" // 本地化命名空间

DEFINE_LOG_CATEGORY(LogCesium); // 定义 Cesium 专用的日志分类 LogCesium

void FCesiumRuntimeModule::StartupModule() {
  Cesium3DTilesContent::registerAllTileContentTypes(); // 注册所有 3D Tiles 瓦片内容类型（Batched3DModel, Instanced3DModel 等）

  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger(); // 获取 spdlog 默认日志记录器
  pLogger->sinks() = {std::make_shared<SpdlogUnrealLoggerSink>()}; // 将 sink 替换为 UE 日志桥接器

  FModuleManager::Get().LoadModuleChecked(TEXT("HTTP")); // 确保 HTTP 模块已加载

  CESIUM_TRACE_INIT( // 初始化性能追踪，生成带时间戳的 trace 文件名
      "cesium-trace-" +
      std::to_string(std::chrono::time_point_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count()) +
      ".json");

  FString PluginShaderDir = FPaths::Combine( // 拼接插件着色器目录
      IPluginManager::Get().FindPlugin(TEXT("CesiumForUnreal"))->GetBaseDir(), // 获取插件根目录
      TEXT("Shaders")); // 着色器子目录
  AddShaderSourceDirectoryMapping( // 注册虚拟着色器路径映射
      TEXT("/Plugin/CesiumForUnreal"), // 虚拟路径前缀
      PluginShaderDir); // 实际磁盘路径
}

void FCesiumRuntimeModule::ShutdownModule() { CESIUM_TRACE_SHUTDOWN(); } // 关闭性能追踪

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime) // 注册为 CesiumRuntime 模块的入口

// --- 全局 Troubleshooting 委托 ---
FCesium3DTilesetIonTroubleshooting OnCesium3DTilesetIonTroubleshooting{}; // 3D Tileset Ion 连接问题诊断委托
FCesiumRasterOverlayIonTroubleshooting
    OnCesiumRasterOverlayIonTroubleshooting{}; // Raster Overlay Ion 连接问题诊断委托

// 获取全局异步系统单例 — 基于 UnrealTaskProcessor
CesiumAsync::AsyncSystem& getAsyncSystem() noexcept {
  static CesiumAsync::AsyncSystem asyncSystem( // 静态局部变量保证单例
      std::make_shared<UnrealTaskProcessor>()); // 使用 UE 任务图作为线程池
  return asyncSystem;
}

namespace {

// 获取缓存数据库路径 — 跨平台兼容
std::string getCacheDatabaseName() {
#if PLATFORM_ANDROID
  FString BaseDirectory = FPaths::ProjectPersistentDownloadDir(); // Android：外部存储持久目录
#elif PLATFORM_IOS
  FString BaseDirectory =
      FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Cesium")); // iOS：Saved/Cesium
  if (!IFileManager::Get().DirectoryExists(*BaseDirectory)) { // 确保目录存在
    IFileManager::Get().MakeDirectory(*BaseDirectory, true);
  }
#else
  FString BaseDirectory = FPaths::ProjectUserDir(); // 桌面平台：项目 User 目录
#endif

  FString CesiumDBFile =
      FPaths::Combine(*BaseDirectory, TEXT("cesium-request-cache.sqlite")); // 缓存数据库文件名
  FString PlatformAbsolutePath =
      IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite( // 转换为外部应用可写的绝对路径
          *CesiumDBFile);

  UE_LOG( // 日志输出缓存路径
      LogCesium,
      Display,
      TEXT("Caching Cesium requests in %s"),
      *PlatformAbsolutePath);

  return TCHAR_TO_UTF8(*PlatformAbsolutePath); // 返回 UTF-8 编码的路径字符串
}

} // namespace

// 获取全局 SQLite 缓存数据库单例
std::shared_ptr<CesiumAsync::ICacheDatabase>& getCacheDatabase() {
  static int MaxCacheItems =
      GetDefault<UCesiumRuntimeSettings>()->MaxCacheItems; // 从项目设置读取最大缓存条目数

  static std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      std::make_shared<CesiumAsync::SqliteCache>( // 创建 SQLite 缓存
          spdlog::default_logger(), // spdlog 日志
          getCacheDatabaseName(),  // 数据库文件路径
          MaxCacheItems); // 缓存条目上限

  return pCacheDatabase;
}

// 获取全局资产访问器单例（带缓存 + Gzip 解压）
const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() {
  static int RequestsPerCachePrune =
      GetDefault<UCesiumRuntimeSettings>()->RequestsPerCachePrune; // 每次缓存修剪的请求数
  static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumAsync::GunzipAssetAccessor>( // 外层：Gzip 解压
          std::make_shared<CesiumAsync::CachingAssetAccessor>( // 中层：缓存
              spdlog::default_logger(),
              std::make_shared<UnrealAssetAccessor>(), // 内核：UE 平台 HTTP/文件访问
              getCacheDatabase(), // SQLite 缓存
              RequestsPerCachePrune)); // 修剪间隔
  return pAssetAccessor;
}