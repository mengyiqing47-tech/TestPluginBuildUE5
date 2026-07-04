// Copyright Epic Games, Inc. All Rights Reserved.

#include "AGlobalUtil.h" // 引入 AGlobalUtil 模块头文件

#define LOCTEXT_NAMESPACE "FAGlobalUtilModule" // 定义本地化命名空间

void FAGlobalUtilModule::StartupModule()
{
	// 模块加载后执行此处代码 — 当前无自定义初始化逻辑
}

void FAGlobalUtilModule::ShutdownModule()
{
	// 模块卸载前执行清理 — 当前无自定义清理逻辑
}

#undef LOCTEXT_NAMESPACE // 结束本地化命名空间
	
IMPLEMENT_MODULE(FAGlobalUtilModule, AGlobalUtil) // 将 FAGlobalUtilModule 注册为 AGlobalUtil 插件的模块入口