
#include "ShareMemory/UtilShareMemory.h" // 本类头文件

#ifdef _WIN32 // Windows 平台：使用 File Mapping 实现共享内存
#include "Windows/AllowWindowsPlatformTypes.h" // 暂时允许 Windows 原生类型
#include <windows.h> // Windows API：CreateFileMapping, OpenFileMapping, MapViewOfFile
#include "Windows/HideWindowsPlatformTypes.h" // 恢复 UE 对 Windows 类型的屏蔽
#else // Linux 平台：使用 System V 共享内存
#include<sys/types.h> // key_t 类型定义
#include<sys/shm.h> // Linux 共享内存 API：shmget, shmat, shmdt, shmctl
#endif

#ifdef _WIN32
// 将 UTF-8 编码的 std::string 转换为 Windows API 需要的宽字符串 LPCWSTR
LPCWSTR StringToLPCWSTR(const std::string& s) {
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0); // 第一次调用获取所需缓冲区大小
	wchar_t* buffer = new wchar_t[size]; // 分配宽字符缓冲区
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, size); // 执行 UTF-8 到 UTF-16 转换
	return buffer; // 返回宽字符串指针（调用方负责释放）
}
#endif

// 不透明句柄结构体：封装平台相关的共享内存句柄与映射指针
struct UtilShareMemoryHandle
{
#ifdef _WIN32
    HANDLE file_mapper = NULL;        // 文件映射对象句柄（CreateFileMapping / OpenFileMapping 返回）
    LPVOID sharememory_pointer = NULL; // 映射到进程地址空间的视图指针
#else
    key_t key = -1;                   // System V IPC key（由名称哈希生成）
    int id = -1;                      // 共享内存段 ID（shmget 返回）
    void* sharememory_pointer = NULL; // 映射到进程地址空间的指针（shmat 返回）
#endif
};

UtilShareMemory::UtilShareMemory()
    : Name("")        // 默认空名称
	, Handle(NULL)    // 句柄结构体稍后在 Init() 中分配
{
	Init(); // 分配 Handle 结构体
}

UtilShareMemory::UtilShareMemory(std::string Name)
    : Name(Name)      // 指定共享内存名称
    ,Handle(NULL)
{
    Init(); // 分配 Handle 结构体
}

UtilShareMemory::~UtilShareMemory()
{
    Release(); // 释放所有资源（解除映射、关闭句柄、删除结构体）
}

std::string UtilShareMemory::GetName()
{
    return Name; // 返回共享内存的名称标识
}

void UtilShareMemory::SetName(std::string InName)
{
    ResetHandle();     // 先关闭当前已打开的共享内存
    this->Name = InName; // 更新为新的名称
}

bool UtilShareMemory::Create(unsigned int size)
{
    if(size <= 0) // 创建的共享内存大小必须大于 0
    {
        printf("[PShareMemory::create] can not create sharememory,"
                "size you pass must greater than 0.\n"); // 向控制台输出错误信息
        return false;
    }

#ifdef _WIN32
    // 共享内存已存在的情况：通常是先 Open 成功后再次 Create
    if(NULL != Handle->file_mapper)
    {
        printf("[PShareMemory::create] the sharememory you want is existed.\n");
        return false;
    }

	LPCWSTR tName = StringToLPCWSTR(Name); // 将 UTF-8 名称转换为宽字符串

    Handle->file_mapper = ::CreateFileMapping(INVALID_HANDLE_VALUE, // 使用系统分页文件（非实际文件映射）
                                                             NULL, // 默认安全属性
                                                   PAGE_READWRITE, // 可读可写权限
                                                                0, // 高位大小（0 表示由 size 决定全部）
                                                             size, // 映射大小（字节）
															tName); // 映射对象名称
	delete tName; // 释放临时的宽字符缓冲区

    if(NULL == Handle->file_mapper || GetLastError() == ERROR_ALREADY_EXISTS) // 创建失败或同名的已存在
    {
        printf("[PShareMemory::create] create sharememory failed,"
               "maybe it is existed or an unkown error occurs.\n");
        return false;
    }
#else // Linux
    // 共享内存已存在的情况
    if(-1 != Handle->key && -1 != Handle->id )
    {
        printf("[PShareMemory::create] the sharememory you want is existed.\n");
        return false;
    }

    Handle->key = std::hash<std::string>()(Name); // 使用 std::hash 将名称转换为 IPC key
    Handle->id = shmget(Handle->key, size,IPC_CREAT|IPC_EXCL|0666); // 独占创建共享内存段
    if(-1 == Handle->key || -1 == Handle->id) // 创建失败
    {
        printf("[PShareMemory::create] create sharememory failed,"
               "maybe it is existed or an unkown error occurs.\n");
        return false;
    }
#endif

    return true; // 共享内存创建成功
}

void* UtilShareMemory::Open()
{
#ifdef _WIN32
    // 已经映射过了，直接返回已有的指针
    if(NULL != Handle->sharememory_pointer)
    {
        return Handle->sharememory_pointer;
    }
    // 没有 Create 或 Create 失败：尝试打开已存在的共享内存
    if(NULL == Handle->file_mapper)
    {
		LPCWSTR tName = StringToLPCWSTR(Name); // UTF-8 → 宽字符串
        Handle->file_mapper = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, // 完全访问权限
                                                                  0,  // 不继承句柄
															tName);   // 共享内存名称
		delete tName; // 释放临时缓冲区

        if(NULL == Handle->file_mapper) // 打开失败
        {
            printf("[PShareMemory::open] open sharememory failed,"
                   "maybe it is not existed.\n");
            return NULL;
        }
    }

    // 将文件映射对象映射到当前进程地址空间
    Handle->sharememory_pointer = ::MapViewOfFile(Handle->file_mapper,   // 映射对象句柄
                                                  FILE_MAP_ALL_ACCESS,  // 完全访问权限
                                                                    0,  // 文件偏移高 32 位
                                                                    0,  // 文件偏移低 32 位
                                                                    0); // 映射整个文件
    if(NULL == Handle->sharememory_pointer) // 映射失败
    {
        printf("[PShareMemory::open] open sharememory failed with "
               "an unkown error occurs.\n");
    }
    return Handle->sharememory_pointer; // 返回映射后的地址指针
#else // Linux
    // 已经映射过了
    if(NULL != Handle->sharememory_pointer)
    {
        return Handle->sharememory_pointer;
    }
    // 没有 Create 或 Create 失败：尝试打开已存在的共享内存
    if(-1 == Handle->key && -1 == Handle->id)
    {
        Handle->key = std::hash<std::string>()(Name); // 生成 IPC key
        Handle->id = shmget(Handle->key,0,IPC_CREAT|0666); // 打开已存在的段（size=0 不要求大小）
        Handle->sharememory_pointer = shmat(Handle->id,NULL,0); // 附加到进程地址空间（NULL 让系统选地址）

        if(-1 == Handle->key || 
           -1 == Handle->id ||
           (void*)-1 == Handle->sharememory_pointer) // 任何一步失败
        {
            printf("[PShareMemory::open] open sharememory failed,"
                   "maybe it is not existed.\n");
            return NULL;
        }
    }

    Handle->id = shmget(Handle->key,0,IPC_CREAT|0666); // 打开共享内存段
    Handle->sharememory_pointer = shmat(Handle->id,NULL,0); // 附加到进程地址空间
    if(-1 == Handle->id || (void*)-1 == Handle->sharememory_pointer) // 失败
    {
        printf("[PShareMemory::open] open sharememory failed with "
               "an unkown error occurs.\n");
        return NULL;
    }

    return Handle->sharememory_pointer; // 返回映射后的地址指针
#endif
}

// 解除共享内存视图在进程地址空间的映射
void UtilShareMemory::UnMappingFileImp()
{
#ifdef _WIN32
	if (NULL != Handle->sharememory_pointer) { // 确保已映射
		::UnmapViewOfFile(Handle->sharememory_pointer); // Windows：解除视图映射
	}
#else
	if (NULL != Handle->sharememory_pointer) // 确保已映射
	{
		shmdt(Handle->sharememory_pointer); // Linux：分离共享内存段
	}
#endif
}

// 关闭平台相关的共享内存底层句柄
void UtilShareMemory::CloseHandleImp()
{
#ifdef _WIN32
    if(NULL != Handle->file_mapper) // 确保句柄有效
    {
        ::CloseHandle(Handle->file_mapper); // Windows：关闭文件映射句柄
    }
#else
    if(-1 != Handle->id) // 确保共享内存段存在
    {
        shmctl(Handle->id, IPC_RMID,NULL); // Linux：标记共享内存段为待删除
    }
#endif
}

// 重置 Handle 结构体内的平台参数为初始值
void UtilShareMemory::ResetParamImp()
{
#ifdef _WIN32
	Handle->file_mapper = NULL;         // 清除文件映射句柄
	Handle->sharememory_pointer = NULL; // 清除映射指针
#else
	Handle->key = -1;                  // 重置 IPC key
	Handle->id = -1;                   // 重置共享内存段 ID
	Handle->sharememory_pointer = NULL; // 清除映射指针
#endif 
}

// 初始化 Handle 结构体（分配内存）
void UtilShareMemory::Init()
{
    if(NULL == Handle) // 防止重复分配
    {
        Handle = new UtilShareMemoryHandle; // 在堆上分配不透明句柄结构体
    }
}

// 释放所有资源并销毁 Handle
void UtilShareMemory::Release()
{
    if(NULL != Handle) // 确保已分配
    {
		ResetHandle();  // 解除映射 + 关闭句柄 + 重置参数
        delete Handle;  // 释放 Handle 结构体内存
		Handle = NULL;  // 置空指针防止悬垂引用
    }
}

// 完整重置共享内存句柄：解除映射 → 关闭句柄 → 重置参数
void UtilShareMemory::ResetHandle()
{
	if (NULL != Handle) { // 确保 Handle 存在
		UnMappingFileImp(); // 步骤 1：解除内存映射
		CloseHandleImp();   // 步骤 2：关闭底层句柄
		ResetParamImp();    // 步骤 3：重置参数为初始值
	}
}