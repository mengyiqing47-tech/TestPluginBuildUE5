// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


//按位操作
#define Msetbit(x,y)  x|=(1<<y)//设置y位为1
#define Mclrbit(x,y)  x&=~(1<<y)//设置y位为0
#define Mreversebit(x,y) x^=(1<<y)//取反y位
#define Mgetbit(x,y)  ((x)>>(y)&1)//获取y位

class FAGlobalUtilModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
