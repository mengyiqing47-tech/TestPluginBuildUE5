// Copyright Epic Games, Inc. All Rights Reserved.

#include "AWeatherSystem.h" // 引入 AWeatherSystem 模块头文件

#define LOCTEXT_NAMESPACE "FAWeatherSystemModule" // 定义本地化命名空间

void FAWeatherSystemModule::StartupModule()
{
	// 模块加载后执行 — 当前无自定义初始化逻辑
}

void FAWeatherSystemModule::ShutdownModule()
{
	// 模块卸载前执行清理 — 当前无自定义清理逻辑
}

#undef LOCTEXT_NAMESPACE // 结束本地化命名空间
	
IMPLEMENT_MODULE(FAWeatherSystemModule, AWeatherSystem) // 将 FAWeatherSystemModule 注册为 AWeatherSystem 插件的模块入口