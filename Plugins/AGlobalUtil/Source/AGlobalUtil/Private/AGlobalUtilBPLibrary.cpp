// Copyright Epic Games, Inc. All Rights Reserved.

#include "AGlobalUtilBPLibrary.h" // 本类头文件
#include "AGlobalUtil.h" // 位操作宏 Msetbit/Mclrbit/Mgetbit 定义
#include "IImageWrapperModule.h" // 图片编解码模块接口
#include "IImageWrapper.h" // 图片编解码封装器
#include "Engine/TextureRenderTarget2D.h" // UTextureRenderTarget2D 渲染目标
#include "Developer/DirectoryWatcher/Public/DirectoryWatcherModule.h" // 目录监视器模块
#include "Developer/DirectoryWatcher/Public/IDirectoryWatcher.h" // 目录监视器接口
#include "HAL/UnrealMemory.h" // FMemory 内存操作
#include "Misc/FileHelper.h" // FFileHelper 文件操作
#include "Kismet/GameplayStatics.h" // UGameplayStatics 游戏静态函数
#include "Async/Async.h" // AsyncTask 异步任务
#include "Engine/Engine.h" // GEngine 引擎全局句柄
#include "Engine/World.h" // GWorld/UWorld 世界上下文
#include "Engine/Texture2D.h" // UTexture2D 2D 纹理

#if WITH_EDITOR
#include "Editor.h" // GEditor 编辑器全局句柄
#include "EditorViewportClient.h" // FEditorViewportClient 编辑器视口客户端
#include "LevelEditorViewport.h" // FViewport 视口
#endif

FPFileChangeData::FPFileChangeData(const FFileChangeData& Data)
{
	Filename = Data.Filename; // 拷贝文件名
	Action = EPFileChangeAction(Data.Action); // 将原生 FFileChangeData::EAction 显式转换为 EPFileChangeAction
}



TMap<FPFileChangeDelegateInfo, FDelegateHandle> UAGlobalUtilBPLibrary::DirectoryWatcherHandleMap; // 静态成员：存储所有已注册的目录监视器委托句柄


UAGlobalUtilBPLibrary::UAGlobalUtilBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer) // 调用父类 UBlueprintFunctionLibrary 构造函数
{

}


void UAGlobalUtilBPLibrary::ShowMsgLog(FString Msg)
{
	if (GEngine) // 确保 GEngine 全局对象有效
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Msg); // 在屏幕上显示红色调试消息，持续 5 秒，Key=-1 表示不覆盖旧消息
	UE_LOG(LogTemp, Display, TEXT("%s"), *Msg); // 同时输出到日志系统
}

FString UAGlobalUtilBPLibrary::CalculateFileMD5(const FString& FilePath, int32 ChunkSize)
{
	// 获取默认文件管理器
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 检查文件是否存在
	if (!PlatformFile.FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File Not Exists: %s"), *FilePath); // 日志记录文件不存在错误
		return FString(); // 返回空字符串表示计算失败
	}

	// 打开文件读取流
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));
	if (!FileHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Open File: %s"), *FilePath); // 日志记录文件打开失败错误
		return FString();
	}

	// 初始化 MD5 上下文
	FMD5 MD5;
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(ChunkSize); // 分配指定大小的未初始化缓冲区

	// 分块读取文件内容
	while (true)
	{
		int32 BytesRead = FileHandle->Read(Buffer.GetData(), ChunkSize); // 读取一块数据（默认 8KB）
		if (BytesRead <= 0) // 读到文件末尾，退出循环
		{
			break;
		}
		// 更新 MD5 计算
		MD5.Update(Buffer.GetData(), BytesRead);
	}

	// 完成 MD5 计算并获取结果摘要
	uint8 Digest[16]; // MD5 输出 16 字节（128 位）
	MD5.Final(Digest);

	// 转换 MD5 结果为十六进制字符串
	FString MD5Hash;
	for (int32 i = 0; i < 16; ++i)
	{
		MD5Hash += FString::Printf(TEXT("%02x"), Digest[i]); // 逐字节格式化为两位十六进制小写字符
	}

	return MD5Hash; // 返回完整的 32 位十六进制 MD5 字符串
}

void UAGlobalUtilBPLibrary::SetBit8(uint8 InVal, int32 InBit, bool InStat)
{
	if (InStat) { // 需要设置位为 1
		Msetbit(InVal, InBit); // 调用宏将指定位设为 1
	}
	else { // 需要设置位为 0
		Mclrbit(InVal, InBit); // 调用宏将指定位设为 0
	}
}

void UAGlobalUtilBPLibrary::SetBit32(int32 InVal, int32 InBit, bool InStat)
{
	if (InStat) { // 需要设置位为 1
		Msetbit(InVal, InBit);
	}
	else { // 需要设置位为 0
		Mclrbit(InVal, InBit);
	}
}

bool UAGlobalUtilBPLibrary::GetBit8(uint8 InVal, int32 InBit)
{
	return Mgetbit(InVal, InBit); // 获取 uint8 值的指定位，返回 bool（0 或 1）
}

bool UAGlobalUtilBPLibrary::GetBit32(int32 InVal, int32 InBit)
{
	return Mgetbit(InVal, InBit); // 获取 int32 值的指定位
}

TArray<bool> UAGlobalUtilBPLibrary::GetBitAll8(uint8 InVal)
{
	TArray<bool> OutStat;
	for (int i = 0; i < 8; i++) { // 从右到左遍历 8 位（LSB 在 index=0）
		OutStat.Add(GetBit8(InVal, i)); // 按位获取
	}
	return OutStat; // 返回 8 位布尔数组
}

TArray<bool> UAGlobalUtilBPLibrary::GetBitAll32(int32 InVal)
{
	TArray<bool> OutStat;
	for (int i = 0; i < 32; i++) { // 从右到左遍历 32 位（LSB 在 index=0）
		OutStat.Add(GetBit32(InVal, i));
	}
	return OutStat; // 返回 32 位布尔数组
}

EPensorImageFormat UAGlobalUtilBPLibrary::GetImageFormatByPath(const FString& FilePath)
{
	EPensorImageFormat TargetSavFmt = EPensorImageFormat::Invalid; // 默认无效格式
	if (FilePath.EndsWith(".png")) // 判断文件扩展名是否 .png
	{
		TargetSavFmt = EPensorImageFormat::PNG; // 设为 PNG 格式
	}
	else if (FilePath.EndsWith(".jpg")) // 判断 .jpg 扩展名
	{
		TargetSavFmt = EPensorImageFormat::JPEG; // 设为 JPEG 格式
	}
	else if (FilePath.EndsWith(".bmp")) // 判断 .bmp 扩展名
	{
		TargetSavFmt = EPensorImageFormat::BMP; // 设为 BMP 格式
	}
	return TargetSavFmt; // 未匹配则返回 Invalid
}

bool UAGlobalUtilBPLibrary::SaveRT(UTextureRenderTarget2D* RT, const FString& SavePath)
{
	if (!RT) { return false; } // 渲染目标为空则失败
	IImageWrapperModule* ImageWrapperModule = FModuleManager::GetModulePtr<IImageWrapperModule>(FName("ImageWrapper")); // 获取 ImageWrapper 模块指针
	if (ImageWrapperModule == nullptr) // 模块不可用
	{
		return false;
	}
	int32 tWidth = RT->SizeX; // 渲染目标宽度（像素）
	int32 tHeight = RT->SizeY; // 渲染目标高度（像素）
	TArray<FColor> RTColor; // 存储读取到的像素颜色数组
	FRenderTarget* FRT = RT->GameThread_GetRenderTargetResource(); // 获取渲染目标资源指针（仅游戏线程安全）
	if (FRT && FRT->ReadPixels(RTColor)) // 读取渲染目标的像素数据到 RTColor 数组
	{
		// 在后台线程异步执行图片保存，不阻塞游戏/渲染线程
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ImageWrapperModule, RTColor, tWidth, tHeight, SavePath]
		{
			EImageFormat TargetSavFmt = EImageFormat(GetImageFormatByPath(SavePath)); // 根据保存路径扩展名确定编码格式
			if (EImageFormat::BMP == TargetSavFmt) // BMP 格式：使用引擎内置位图创建函数
			{
				FFileHelper::CreateBitmap(*SavePath, tWidth, tHeight, RTColor.GetData()); // 直接创建 BMP 文件
				return true; // 保存成功
			}
			else if(EImageFormat::JPEG == TargetSavFmt || EImageFormat::PNG == TargetSavFmt) // JPEG 或 PNG 格式：使用 ImageWrapper 编码
			{
				TArray<uint8> RawBuf; // 原始 BGRA 字节缓冲区
				for (const FColor& PixelColor : RTColor) // 遍历每个像素
				{
					RawBuf.Add(PixelColor.B); // B 通道（BGRA 顺序）
					RawBuf.Add(PixelColor.G); // G 通道
					RawBuf.Add(PixelColor.R); // R 通道
					RawBuf.Add(PixelColor.A); // A 通道
				}
				TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule->CreateImageWrapper(TargetSavFmt); // 创建对应格式的编码器
				if (ImageWrapper) // 编码器创建成功
				{
					ImageWrapper->SetRaw(RawBuf.GetData(), RawBuf.Num(), tWidth, tHeight, ERGBFormat::BGRA, 8); // 设置原始 BGRA 像素数据
					FFileHelper::SaveArrayToFile(ImageWrapper->GetCompressed(), *SavePath); // 编码压缩后写入文件
				}
			}
			return true; // 任务完成
		});
		return true; // 异步任务已启动视为成功
	}
	return false; // 读取像素数据失败
}

bool UAGlobalUtilBPLibrary::SaveColorData(FPImgColorData Data, const FString& SavePath) // 蓝图版本：从蓝图结构体保存图片到文件
{
	// 将蓝图结构体转换为 C++ 原生结构体
	FPCImgColorData CData;
	CData.Width = Data.Width; // 拷贝图像宽度
	CData.Height = Data.Height; // 拷贝图像高度
	CData.ImgPixelFmt = (EUtilRGBFormat)Data.ImgPixelFmt; // 拷贝像素格式枚举（显式转换）
	CData.ImgData = Data.ImgData.GetData(); // 获取 TArray 内部原始指针（需要生命周期保护）
	return CSaveColorData(CData, SavePath); // 委托给 C++ 原生版本保存
}


bool UAGlobalUtilBPLibrary::CSaveColorData(FPCImgColorData Data, const FString& SavePath) // C++ 原生版本：从原始数据保存图片
{
	IImageWrapperModule* ImageWrapperModule = FModuleManager::GetModulePtr<IImageWrapperModule>(FName("ImageWrapper")); // 获取 ImageWrapper 模块
	if (ImageWrapperModule == nullptr) // 模块不可用
	{
		return false;
	}
	EImageFormat TargetSavFmt = EImageFormat(GetImageFormatByPath(SavePath)); // 根据路径扩展名确定编码格式

	if (EImageFormat::BMP == TargetSavFmt) // BMP 格式处理
	{
		if (EUtilRGBFormat::RGBA == Data.ImgPixelFmt || EUtilRGBFormat::BGRA == Data.ImgPixelFmt) // 仅支持 RGBA/BGRA 4 通道像素格式
		{
			TArray<FColor> RTColor; // 用于 CreateBitmap 的 FColor 数组
			RTColor.SetNum(Data.Width * Data.Height); // 预分配像素数量空间
			for (int i = 0; i < Data.Width * Data.Height; i++)
			{
				RTColor[i].R = Data.ImgData[i * 4 + 0]; // 像素第 1 字节（RGBA 时为 R，BGRA 时为 B — 不区分，直接按位置拷贝）
				RTColor[i].G = Data.ImgData[i * 4 + 1]; // 像素第 2 字节（G）
				RTColor[i].B = Data.ImgData[i * 4 + 2]; // 像素第 3 字节（RGBA 时为 B，BGRA 时为 R）
				RTColor[i].A = Data.ImgData[i * 4 + 3]; // 像素第 4 字节（A）
			}
			FFileHelper::CreateBitmap(*SavePath, Data.Width, Data.Height, RTColor.GetData()); // 创建 BMP 文件
		}
		return false; // BMP 不支持灰度等非 4 通道格式
	}
	else if (EImageFormat::JPEG == TargetSavFmt || EImageFormat::PNG == TargetSavFmt) // JPEG/PNG 格式处理
	{
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule->CreateImageWrapper(TargetSavFmt); // 创建编码器
		if (ImageWrapper)
		{
			ERGBFormat Fmt = (Data.ImgPixelFmt == EUtilRGBFormat::RGBA) ? ERGBFormat::RGBA : ERGBFormat::BGRA; // 根据像素格式映射 ERGBFormat
			ImageWrapper->SetRaw(Data.ImgData, Data.Width * Data.Height * 4, Data.Width, Data.Height, Fmt, 8); // 设置原始像素数据
			FFileHelper::SaveArrayToFile(ImageWrapper->GetCompressed(), *SavePath); // 编码并写入文件
		}
	}
	return false; // 其他格式暂不支持
}

UTexture2D* UAGlobalUtilBPLibrary::LoadImgToTexture2D(const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath)) { return nullptr; } // 文件不存在则返回空指针
	UTexture2D* OutTexture = nullptr; // 输出纹理指针初始化为空
	IImageWrapperModule* ImgWrapModule = FModuleManager::GetModulePtr<IImageWrapperModule>("ImageWrapper"); // 获取 ImageWrapper 模块
	if (ImgWrapModule) // 模块可用
	{
		EImageFormat ImgFmt = EImageFormat(GetImageFormatByPath(FilePath)); // 根据路径扩展名确定图片解码格式
		TSharedPtr<IImageWrapper> ImgWrap = ImgWrapModule->CreateImageWrapper(ImgFmt); // 创建对应格式的解码器
		if (ImgWrap) // 解码器创建成功
		{
			TArray<uint8> ImgData; // 文件原始字节缓冲区
			FFileHelper::LoadFileToArray(ImgData, *FilePath); // 从磁盘加载完整文件到内存
			if (ImgWrap->SetCompressed((void*)ImgData.GetData(), ImgData.Num())) // 设置压缩数据并内部解压
			{
				TArray<uint8> RawData; // 解压后的像素数据
				ImgWrap->GetRaw(ERGBFormat::BGRA, 8, RawData); // 以 BGRA 格式提取 8 位通道的解压数据
				OutTexture = UTexture2D::CreateTransient(ImgWrap->GetWidth(), ImgWrap->GetHeight()); // 创建临时 2D 纹理
				FTexturePlatformData* PlatformData = OutTexture->GetPlatformData(); // 获取纹理平台数据对象
				if (!PlatformData) // 平台数据不可用
					return nullptr;
				// 确保 mip 级存在
				if (PlatformData->Mips.Num() != 0)
				{
					FTexture2DMipMap& Mip = PlatformData->Mips[0]; // 获取第 0 级 mip（全分辨率）
					// 在 BulkData 中重新分配空间并复制像素数据
					void* MipData = Mip.BulkData.Realloc(RawData.Num()); // 按解压数据大小在 BulkData 中分配
					if (MipData)
					{
						FMemory::Memcpy(MipData, RawData.GetData(), RawData.Num()); // 拷贝像素数据到 BulkData
						// 更新资源到渲染线程
						OutTexture->UpdateResource(); // 通知渲染线程同步纹理资源
					}
				}
				// 以下为过时的 Lock/Unlock API，保留作为参考：
				//void* MipData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				//FMemory::Memcpy(MipData, RawData.GetData(), RawData.Num());
				//OutTexture->PlatformData->Mips[0].BulkData.Unlock();
				//OutTexture->UpdateResource();
			}
		}
	}
	return OutTexture; // 返回创建的纹理，失败则返回 nullptr
}

void UAGlobalUtilBPLibrary::ListDir(const FString& DirPath, TArray<FString>& OutFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile(); // 获取平台文件操作接口
	if (FPaths::DirectoryExists(DirPath)) // 检查目录是否存在
	{
		PlatformFile.FindFilesRecursively(OutFiles, *DirPath, nullptr); // 递归列出该目录下所有文件（nullptr 表示不按扩展名筛选）
	}
}

bool UAGlobalUtilBPLibrary::DeleteFile(const FString& FilePath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile(); // 获取平台文件操作接口
	if (PlatformFile.FileExists(*FilePath)) // 检查文件是否存在
	{
		PlatformFile.DeleteFile(*FilePath); // 删除文件
		return true; // 删除成功
	}
	return false; // 文件不存在
}

void UAGlobalUtilBPLibrary::RegisterDirectoryWatcher(const FPFileChangeDelegateInfo& Info, FPDirectoryChanged DirectoryChangedDelegate)
{
	UnRegisterDirectoryWatcher(Info); // 先尝试注销同名委托，防止重复注册导致资源泄漏
	FDelegateHandle tHandle; // 委托句柄，由 RegisterDirectoryChangedCallback_Handle 输出
	FDirectoryWatcherModule& DirWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")); // 加载目录监视器模块
	IDirectoryWatcher* DirWatcher = DirWatcherModule.Get(); // 获取目录监视器接口
	if (DirWatcher) // 接口有效
	{
		// 注册目录变更回调 — Lambda 将原生 FFileChangeData 转换为蓝图友好的 FPFileChangeData 再触发动态委托
		DirWatcher->RegisterDirectoryChangedCallback_Handle(Info.WatcherPath,
			IDirectoryWatcher::FDirectoryChanged::CreateLambda([DirectoryChangedDelegate](const TArray<FFileChangeData>& FileChangeDataList) {
			TArray<FPFileChangeData> OutData; // 转换后的变更数组
			for (auto FileChangeData : FileChangeDataList) // 遍历原生变更列表
			{
				FPFileChangeData tChangeData(FileChangeData); // 通过构造函数转换每一个变更项
				OutData.Add(tChangeData);
			}
			if (DirectoryChangedDelegate.IsBound()) // 检查蓝图委托是否已绑定
			{
				DirectoryChangedDelegate.Execute(OutData); // 执行蓝图委托，传递转换后的数据
			}
		}),
			tHandle // 输出句柄供后续注销使用
		);
		DirectoryWatcherHandleMap.Add(Info, tHandle); // 将委托信息与句柄存入映射表
	}
}

void UAGlobalUtilBPLibrary::UnRegisterDirectoryWatcher(const FPFileChangeDelegateInfo& Info)
{
	if (DirectoryWatcherHandleMap.Contains(Info)) // 检查映射表中是否存在该监视器的注册信息
	{
		FDelegateHandle tHandle = DirectoryWatcherHandleMap[Info]; // 取出对应的委托句柄
		FDirectoryWatcherModule& DirWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")); // 加载目录监视器模块
		IDirectoryWatcher* DirWatcher = DirWatcherModule.Get(); // 获取目录监视器接口
		if (DirWatcher) // 接口有效
		{
			DirWatcher->UnregisterDirectoryChangedCallback_Handle(Info.WatcherPath, tHandle); // 注销该路径上的变更回调
		}
	}
}

FString UAGlobalUtilBPLibrary::GetNowDateStr(const FString& Format)
{
	FDateTime NowTime = FDateTime::Now(); // 获取当前系统时间
	return NowTime.ToString(*Format); // 按指定格式化输出，默认 "%Y%m%d%H%M%S%s" 即年月日时分秒毫秒
}

FIntVector UAGlobalUtilBPLibrary::BPGetWorldOrigin()
{
	return GWorld->OriginLocation; // 返回世界坐标原点偏移量（用于大世界坐标重定位）
}

FTransform UAGlobalUtilBPLibrary::BPGetViewpointTransform()
{
#if WITH_EDITOR // 仅在编辑器环境下编译此代码块
	FViewport* CurViewport = GEditor->GetActiveViewport(); // 获取当前活跃编辑器视口
	if (CurViewport) {
		auto AllViewportClients = GEditor->GetAllViewportClients(); // 获取所有视口客户端列表
		for (auto ViewportClient : AllViewportClients) // 遍历匹配活跃视口的客户端
		{
			if (ViewportClient && ViewportClient == CurViewport->GetClient()) // 找到匹配活跃视口的客户端
			{
				return FTransform(ViewportClient->GetViewRotation(), ViewportClient->GetViewLocation()); // 返回编辑器视点的旋转和位置构成的变换
			}
		}
	}
#endif
	// 运行时路径：获取玩家摄像机管理器的变换
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GWorld, 0); // 获取索引 0 的玩家控制器
	if (PlayerController)
		return PlayerController->PlayerCameraManager->GetActorTransform(); // 返回摄像机管理器的 Actor 变换
	return FTransform::Identity; // 兜底返回单位变换
}

FTransform UAGlobalUtilBPLibrary::BPGetPlayerPawnTransform()
{
#if WITH_EDITOR // 仅在编辑器环境下编译
	FViewport* CurViewport = GEditor->GetActiveViewport(); // 获取当前活跃编辑器视口
	auto AllViewportClients = GEditor->GetAllViewportClients(); // 获取所有视口客户端
	for (auto ViewportClient : AllViewportClients) // 遍历
	{
		if (ViewportClient && ViewportClient == CurViewport->GetClient()) // 匹配活跃视口
		{
			return FTransform(FRotator::ZeroRotator, ViewportClient->GetViewLocation()); // 返回视口位置 + 零旋转构成的变换
		}
	}
#endif
	// 运行时路径：获取玩家 Pawn 的变换
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GWorld, 0); // 获取索引 0 的玩家 Pawn
	if (PlayerPawn)
		return PlayerPawn->GetActorTransform(); // 返回 Pawn 的 Actor 变换
	return FTransform::Identity; // 兜底返回单位变换
}

void UAGlobalUtilBPLibrary::AddCurveFloatPoint(FInterpCurveFloat& Curve, float InVal, float OutVal)
{
	Curve.AddPoint(InVal, OutVal); // 向浮点数插值曲线添加 (输入值, 输出值) 插值点
}

void UAGlobalUtilBPLibrary::ClearCurveFloatPoint(FInterpCurveFloat& Curve)
{
	Curve.Reset(); // 清空浮点数插值曲线的所有插值点
}

float UAGlobalUtilBPLibrary::GetCurveFloatValue(FInterpCurveFloat& Curve, float InVal, float DefaultVal)
{
	return Curve.Eval(InVal, DefaultVal); // 在给定输入处评估曲线，若越界则返回默认值
}

void UAGlobalUtilBPLibrary::AddCurveVectorPoint(FInterpCurveVector& Curve, float InVal, FVector OutVal)
{
	Curve.AddPoint(InVal, OutVal); // 向向量插值曲线添加 (输入值, 输出向量) 插值点
}

void UAGlobalUtilBPLibrary::ClearCurveVectorPoint(FInterpCurveVector& Curve)
{
	Curve.Reset(); // 清空向量插值曲线的所有插值点
}

FVector UAGlobalUtilBPLibrary::GetCurveVectorValue(FInterpCurveVector& Curve, float InVal, FVector DefaultVal)
{
	return Curve.Eval(InVal, DefaultVal); // 在给定输入处评估向量曲线，若越界则返回默认向量
}