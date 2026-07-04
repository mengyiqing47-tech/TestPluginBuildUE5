# 主项目模块 TestPluginBuildUE5

## 文件位置

- [TestPluginBuildUE5.h](../Source/TestPluginBuildUE5/TestPluginBuildUE5.h)
- [TestPluginBuildUE5.cpp](../Source/TestPluginBuildUE5/TestPluginBuildUE5.cpp)

---

## 类分析

### FDefaultGameModuleImpl

| 属性 | 说明 |
|------|------|
| **所在文件** | `TestPluginBuildUE5.cpp` |
| **基类** | `FDefaultGameModuleImpl`（UE 标准默认模块实现） |
| **分类** | 模块入口类（非 UObject） |
| **API 导出** | 无（主模块自动导出） |

#### 职责范围

作为 UE5 游戏工程的**主模块入口**，只做一件事：通过 `IMPLEMENT_PRIMARY_GAME_MODULE` 宏将本模块注册为游戏主模块。没有任何自定义逻辑、重写方法或额外初始化代码。

- `TestPluginBuildUE5.h` 是一个空头文件，仅 `#include "CoreMinimal.h"`。
- `TestPluginBuildUE5.cpp` 只包含模块注册宏。

#### 定义源码

```cpp
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, TestPluginBuildUE5, "TestPluginBuildUE5");
```

---

## 依赖关系

```
TestPluginBuildUE5 (主模块)
 ├── Plugins/AGlobalUtil   ← 通过项目 .uproject / .Build.cs 自动引入
 └── Plugins/AWeatherSystem ← 通过项目 .uproject / .Build.cs 自动引入
```

该模块本身不引用任何插件代码，但项目构建系统会在编译时自动链接两个插件模块。实际的游戏逻辑应在蓝图或插件中实现。

---

> **速评**: 此模块是 UE 自动生成的项目骨架，本身不含业务逻辑。项目的实际功能完全由两个插件 `AGlobalUtil` 和 `AWeatherSystem` 承载。
