# 插件 AGlobalUtil 代码分析

## 文件清单

| 角色 | 路径 |
|------|------|
| 模块入口 | [Private/AGlobalUtil.cpp](../Plugins/AGlobalUtil/Source/AGlobalUtil/Private/AGlobalUtil.cpp) |
| 模块头 | [Public/AGlobalUtil.h](../Plugins/AGlobalUtil/Source/AGlobalUtil/Public/AGlobalUtil.h) |
| INI 配置函数库 | [Public/AGlobalConfigBPLibrary.h](../Plugins/AGlobalUtil/Source/AGlobalUtil/Public/AGlobalConfigBPLibrary.h) |
| INI 配置实现 | [Private/AGlobalConfigBPLibrary.cpp](../Plugins/AGlobalUtil/Source/AGlobalUtil/Private/AGlobalConfigBPLibrary.cpp) |
| 通用工具函数库 | [Public/AGlobalUtilBPLibrary.h](../Plugins/AGlobalUtil/Source/AGlobalUtil/Public/AGlobalUtilBPLibrary.h) |
| 通用工具实现 | [Private/AGlobalUtilBPLibrary.cpp](../Plugins/AGlobalUtil/Source/AGlobalUtil/Private/AGlobalUtilBPLibrary.cpp) |
| 进程锁 | [Public/ProcessLock/UtilProcessLock.h](../Plugins/AGlobalUtil/Source/AGlobalUtil/Public/ProcessLock/UtilProcessLock.h) |
| 进程锁实现 | [Private/ProcessLock/UtilProcessLock.cpp](../Plugins/AGlobalUtil/Source/AGlobalUtil/Private/ProcessLock/UtilProcessLock.cpp) |
| 共享内存 | [Public/ShareMemory/UtilShareMemory.h](../Plugins/AGlobalUtil/Source/AGlobalUtil/Public/ShareMemory/UtilShareMemory.h) |
| 共享内存实现 | [Private/ShareMemory/UtilShareMemory.cpp](../Plugins/AGlobalUtil/Source/AGlobalUtil/Private/ShareMemory/UtilShareMemory.cpp) |

---

## 模块类

### FAGlobalUtilModule

| 属性 | 说明 |
|------|------|
| **基类** | `IModuleInterface` |
| **分类** | UE 插件模块类 |
| **API 导出** | `AGLOBALUTIL_API` |

#### 职责范围

标准 UE 插件模块入口。`StartupModule()` 和 `ShutdownModule()` 当前均为空实现，所有实际功能注册发生在被其他模块使用时（蓝图函数库通过 UCLASS 反射自动注册）。

#### 依赖模块（隐式）

- `ImageWrapper` —— 图片编码/解码
- `DirectoryWatcher` —— 文件系统监控
- 无强制依赖其他自定义插件

---

## 蓝图函数库类

### UGlobalConfigBPLibrary

| 属性 | 说明 |
|------|------|
| **基类** | `UBlueprintFunctionLibrary` |
| **分类** | 蓝图函数库 |
| **所在文件** | `AGlobalConfigBPLibrary.h/.cpp` |
| **Category** | `INIConfig` |

#### 职责范围

提供对 **INI 配置文件** 的 Blueprint 读写接口。所有函数均为静态函数，可在蓝图中直接调用。

#### 核心设计模式

所有读写函数统一签名模式：

```
<类型> 类型名(FString Section, FString Key, FString Path, <入参> Value, int32& retCode);
```

- `tPath` —— 相对于 `ProjectContentDir()` 的文件路径（引擎自动拼接完整路径）
- `retCode` —— 返回值码：`>0` 成功 / `-1` 文件不存在 / `0` 写成功
- 底层通过 `GConfig`（UE 全局配置缓存）操作 INI 文件

#### 函数清单

**基础读写**（Get/Set 配对，共 10 组 20 个函数）：

| 数据类型 | Get 函数 | Set 函数 |
|---------|---------|---------|
| `bool` | `GetConfigBool` | `SetConfigBool` |
| `FString` | `GetConfigString` | `SetConfigString` |
| `FText` | `GetConfigText` | `SetConfigText` |
| `float` | `GetConfigFloat` | `SetConfigFloat` |
| `int32` | `GetConfigInt` | `SetConfigInt` |
| `FVector` | `GetConfigVector` | `SetConfigVector` |
| `FVector2D` | `GetConfigVector2D` | `SetConfigVector2D` |
| `FVector4` | `GetConfigVector4` | `SetConfigVector4` |
| `FColor` | `GetConfigColor` | `SetConfigColor` |
| `FRotator` | `GetConfigRotator` | `SetConfigRotator` |
| `TArray<FString>` | `GetConfigArray` | `SetConfigArray` |

**带默认值的初始化读取**（GetConfigInit 系列，共 11 个函数）：

当配置项不存在时，自动写入默认值到 INI 文件并返回该默认值，确保配置项始终存在。

- `GetConfigInitInt`, `GetConfigInitFloat`, `GetConfigInitString`, `GetConfigInitBool`
- `GetConfigInitColor`, `GetConfigInitRotator`, `GetConfigInitVector4`
- `GetConfigInitVector`, `GetConfigInitVector2D`, `GetConfigInitText`, `GetConfigInitArray`

#### 设计要点

- 使用 `GConfig->Flush()` 确保每次读写前/后文件同步
- 通过 `FPlatformFileManager` 检查文件是否存在，不存在时提前返回 `retCode=-1`
- 只处理标准 INI 格式，不支持自定义 INI 解析

---

### UAGlobalUtilBPLibrary

| 属性 | 说明 |
|------|------|
| **基类** | `UBlueprintFunctionLibrary` |
| **分类** | 蓝图函数库 |
| **所在文件** | `AGlobalUtilBPLibrary.h/.cpp` |
| **Category** | 多个（按功能分组） |

#### 职责范围

综合性工具函数库，涵盖 **日志、MD5 哈希、位操作、图片处理、文件操作、日期工具、视图变换查询、插值曲线操作**。

#### 功能组详细分析

##### 1. 日志工具（MsgLog）

| 函数 | 说明 |
|------|------|
| `ShowMsgLog` | 同时在屏幕（`AddOnScreenDebugMessage`）和日志（`UE_LOG`）输出消息 |

---

##### 2. 哈希工具（Hash）

| 函数 | 说明 |
|------|------|
| `CalculateFileMD5` | 分块（默认 8KB）读取文件计算 MD5 哈希，返回 32 位十六进制字符串 |

使用 `FMD5` 引擎类，支持大文件分块处理。

---

##### 3. 位操作（BitOpr）

| 函数 | 说明 |
|------|------|
| `SetBit8` | 设置 uint8 中指定位为 0 或 1 |
| `SetBit32` | 设置 int32 中指定位为 0 或 1 |
| `GetBit8` | 获取 uint8 中指定位 |
| `GetBit32` | 获取 int32 中指定位 |
| `GetBitAll8` | 获取 uint8 所有位（从右到左） |
| `GetBitAll32` | 获取 int32 所有位（从右到左） |

底层使用 `AGlobalUtil.h` 中定义的宏：

```cpp
#define Msetbit(x,y)  x|=(1<<y)
#define Mclrbit(x,y)  x&=~(1<<y)
#define Mreversebit(x,y) x^=(1<<y)
#define Mgetbit(x,y)  ((x)>>(y)&1)
```

---

##### 4. 图片操作（ImgOpr）

| 函数 | 说明 |
|------|------|
| `GetImageFormatByPath` | 根据文件扩展名确定图片格式（PNG/JPG/BMP/Invalid） |
| `SaveRT` | 保存 `UTextureRenderTarget2D` 到文件（使用 ImageWrapper 模块，支持后台线程） |
| `SaveColorData` | 将 `FPImgColorData`（蓝图结构体）保存为图片文件 |
| `CSaveColorData` | 将 `FPCImgColorData`（C++ 原生指针结构体）保存为图片文件 |
| `LoadImgToTexture2D` | 从磁盘文件加载图片到 `UTexture2D` |

**架构特点**：
- 使用 `IImageWrapperModule`/`IImageWrapper` 处理 PNG/JPEG 编解码
- 保存操作通过 `AsyncTask` 在后台线程执行（非阻塞）
- BMP 格式直接使用 `FFileHelper::CreateBitmap`
- `EUtilRGBFormat` 与 `ERGBFormat` 的转换通过 `(int8)Data.ImgPixelFmt - 1` 实现

---

##### 5. 文件操作（FileOpr）

| 函数 | 说明 |
|------|------|
| `ListDir` | 递归列出目录下所有文件 |
| `DeleteFile` | 删除指定文件 |
| `RegisterDirectoryWatcher` | 注册目录变更监视回调（基于 `FDirectoryWatcherModule`） |
| `UnRegisterDirectoryWatcher` | 注销目录变更监视回调 |

**目录监视器机制**：
- 使用 `FPFileChangeDelegateInfo` 作为键（`DelegateName` + `WatcherPath` 联合哈希）
- 使用静态 `TMap<FPFileChangeDelegateInfo, FDelegateHandle>` 存储所有注册的句柄
- 回调通过 Lambda 将原生 `FFileChangeData` 转换为蓝图友好的 `FPFileChangeData`
- 委托签名：`DECLARE_DYNAMIC_DELEGATE_OneParam(FPDirectoryChanged, const TArray<FPFileChangeData>&, FileChangeLists)`

---

##### 6. 日期工具（DateOpr）

| 函数 | 说明 |
|------|------|
| `GetNowDateStr` | 返回当前时间的格式化字符串，默认格式 `%Y%m%d%H%M%S%s`（年月日时分秒毫秒） |

---

##### 7. 场景查询（GetUtil）

| 函数 | 说明 |
|------|------|
| `BPGetWorldOrigin` | 返回当前世界的 `OriginLocation`（用于大世界坐标偏移） |
| `BPGetViewpointTransform` | 返回编辑器视口或玩家摄像机的变换（优先编辑器视口） |
| `BPGetPlayerPawnTransform` | 返回玩家 Pawn 的变换（位置 + 旋转，优先编辑器视口） |

编辑器视口获取通过 `WITH_EDITOR` 条件编译，使用 `GEditor->GetActiveViewport()` 和视口客户端。

---

##### 8. 插值曲线（PCurves）

| 函数 | 说明 |
|------|------|
| `AddCurveFloatPoint` | 向 `FInterpCurveFloat` 添加插值点 |
| `ClearCurveFloatPoint` | 清空 `FInterpCurveFloat` |
| `GetCurveFloatValue` | 评估 `FInterpCurveFloat` 在给定时间的插值 |
| `AddCurveVectorPoint` | 向 `FInterpCurveVector` 添加插值点 |
| `ClearCurveVectorPoint` | 清空 `FInterpCurveVector` |
| `GetCurveVectorValue` | 评估 `FInterpCurveVector` 在给定时间的插值 |

直接代理到 `FInterpCurve` 内置方法（`AddPoint`、`Reset`、`Eval`）。

#### 静态成员

```cpp
TMap<FPFileChangeDelegateInfo, FDelegateHandle> UAGlobalUtilBPLibrary::DirectoryWatcherHandleMap;
```

用于存储文件监视器的委托句柄，实现注册/注销的映射管理。

#### 依赖模块

- `ImageWrapper` —— 运行时加载的图片编解码模块
- `DirectoryWatcher` —— 运行时加载的文件目录监控模块
- `Engine` / `UnrealEd`（WITH_EDITOR）—— 编辑器视口查询

---

## 跨平台非 UObject 类

### UtilProcessLock

| 属性 | 说明 |
|------|------|
| **API 导出** | `AGLOBALUTIL_API` |
| **分类** | 跨平台进程同步工具 |

#### 职责范围

基于**系统级互斥体**实现进程间同步锁（非线程锁），确保同一时间只有一个进程能访问共享资源。

#### 跨平台实现

| 平台 | 实现方式 |
|------|---------|
| **Win32** | `CreateMutexW` / `WaitForSingleObject` / `ReleaseMutex` |
| **Linux** | System V 信号量 `semget` / `semop` / `semctl` |

#### 核心 API

| 方法 | 说明 |
|------|------|
| `Lock()` | 获取锁（阻塞等待） |
| `UnLock()` | 释放锁 |
| 构造时指定锁名称 | 通过名称在不同进程间标识同一互斥体 |

#### 设计要点

- 锁名称通过 `MultiByteToWideChar` 从 UTF-8 转换为宽字符（Win32 兼容）
- Linux 端使用 `std::hash<std::string>` 将名称哈希为 System V 信号量 key
- 析构时自动清理 Linux 信号量（`semctl(IPC_RMID)`）
- Win32 端 Mutex 句柄在进程退出时由系统自动回收

---

### UtilShareMemory

| 属性 | 说明 |
|------|------|
| **API 导出** | `AGLOBALUTIL_API` |
| **分类** | 跨平台进程间共享内存工具 |

#### 职责范围

基于**系统级共享内存**实现进程间大数据传输。支持创建、打开、读写和管理共享内存块。

#### 跨平台实现

| 平台 | 实现方式 |
|------|---------|
| **Win32** | `CreateFileMapping` / `OpenFileMapping` / `MapViewOfFile` |
| **Linux** | System V 共享内存 `shmget` / `shmat` / `shmdt` / `shmctl` |

#### 核心 API

| 方法 | 说明 |
|------|------|
| `Create(unsigned int size)` | 创建指定大小的共享内存块 |
| `Open()` | 打开已存在的共享内存并返回指针 |
| `GetName()` / `SetName()` | 获取/设置共享内存名称 |
| `ResetHandle()` | 关闭映射和句柄并重置参数 |

#### 架构设计

使用不透明句柄结构 `UtilShareMemoryHandle`（前向声明 + 私有实现）：

```cpp
struct UtilShareMemoryHandle {
#ifdef _WIN32
    HANDLE file_mapper = NULL;
    LPVOID sharememory_pointer = NULL;
#else
    key_t key = -1;
    int id = -1;
    void* sharememory_pointer = NULL;
#endif
};
```

#### 设计要点

- 名称通过 `std::hash<std::string>` 映射到 Linux System V key
- Win32 端名称需转换为 `LPCWSTR`（宽字符）
- `ResetHandle()` 安全关闭三步：解除映射 → 关闭句柄 → 重置参数
- 构造时自动 `Init()`（分配 Handle 结构），析构时自动 `Release()`
- 使用 `printf` 输出日志（非 UE 日志系统，可跨平台使用）

---

## 枚举定义

### EPensorImageFormat

| 值 | 说明 |
|----|------|
| `Invalid (255)` | 无效/未知格式 |
| `PNG (0)` | 便携式网络图形 |
| `JPEG` | JPEG 图片 |
| `GrayscaleJPEG` | 单通道 JPEG |
| `BMP` | Windows 位图 |
| `ICO` | Windows 图标资源 |
| `EXR` | OpenEXR HDR 格式 |
| `ICNS` | Mac 图标 |

---

### EPFileChangeAction

| 值 | 说明 |
|----|------|
| `FCA_Unknown` | 未知变更 |
| `FCA_Added` | 文件添加 |
| `FCA_Modified` | 文件修改 |
| `FCA_Removed` | 文件删除 |

---

### EUtilRGBFormat

| 值 | 说明 |
|----|------|
| `Invalid (0)` | 无效格式 |
| `RGBA (1)` | RGBA 像素顺序 |
| `BGRA (2)` | BGRA 像素顺序（最常见） |
| `Gray (3)` | 灰度单通道 |

---

## 结构体定义

### FPImgColorData（USTRUCT，蓝图可用）

用于在蓝图中传递图像数据的结构体：

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Width` | `int32` | 1024 | 图像宽度 |
| `Height` | `int32` | 1024 | 图像高度 |
| `ImgData` | `TArray<uint8>` | 空 | 图像像素数据 |
| `ImgPixelFmt` | `EUtilRGBFormat` | BGRA | 像素格式 |

---

### FPCImgColorData（纯 C++ 结构体）

C++ 原生版本的图像数据（无 UProperty，使用原始指针）：

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `Width` | `int32` | 1024 | 图像宽度 |
| `Height` | `int32` | 1024 | 图像高度 |
| `ImgData` | `uint8*` | nullptr | 原始像素数据指针 |
| `ImgPixelFmt` | `EUtilRGBFormat` | BGRA | 像素格式 |

---

### FPFileChangeData（USTRUCT）

文件变更通知结构体：

| 字段 | 类型 | 说明 |
|------|------|------|
| `Filename` | `FString` | 变更文件的路径（自动标准化） |
| `Action` | `EPFileChangeAction` | 变更类型 |

支持从 `FFileChangeData` 构造转换。

---

### FPFileChangeDelegateInfo（USTRUCT）

目录监视器注册信息：

| 字段 | 类型 | 说明 |
|------|------|------|
| `DelegateName` | `FString` | 委托标识名称 |
| `WatcherPath` | `FString` | 监视的目录路径 |

实现了 `==`、`!=` 和 `GetTypeHash` 运算符，可作为 `TMap` 键使用。哈希使用 `HashCombine` 组合两个字符串的哈希值。

---

## 全局宏（定义在 AGlobalUtil.h）

```cpp
#define Msetbit(x,y)          x|=(1<<y)         // 设置 y 位为 1
#define Mclrbit(x,y)          x&=~(1<<y)        // 设置 y 位为 0
#define Mreversebit(x,y)      x^=(1<<y)         // 取反 y 位
#define Mgetbit(x,y)          ((x)>>(y)&1)      // 获取 y 位
```

这些宏在 `UAGlobalUtilBPLibrary` 的位操作函数和 `ACWeatherSystem::GetSensorConfig` 中被广泛使用。
