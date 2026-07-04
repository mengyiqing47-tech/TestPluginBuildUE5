
#include "ProcessLock/UtilProcessLock.h" // 本类头文件

#ifdef _WIN32 // Windows 平台：使用系统 Mutex 实现进程锁
#include "Windows/AllowWindowsPlatformTypes.h" // 暂时允许 Windows 类型定义（避免与 UE 类型冲突）
#include <windows.h> // Windows API：CreateMutexW, WaitForSingleObject, ReleaseMutex
#include "Windows/HideWindowsPlatformTypes.h" // 恢复 UE 对 Windows 类型的屏蔽
#elif __linux__ // Linux 平台：使用 System V 信号量实现进程锁
#include<sys/sem.h> // Linux 信号量 API

// System V 信号量控制联合体（Linux 头文件有时不提供此声明）
union semun
{
    int val;                   // SETVAL 使用的值
    struct semid_ds* buf;      // IPC_STAT / IPC_SET 使用的缓冲区
    unsigned short* array;     // GETALL / SETALL 使用的数组
    struct seminfo* __buf;     // IPC_INFO 使用的缓冲区
};

#endif

UtilProcessLock::UtilProcessLock() : Name("") // 默认构造：空名称锁
{
    Init(); // 初始化底层系统锁对象
}

UtilProcessLock::UtilProcessLock(std::string name) : Name(name) // 命名构造：指定名称的进程间锁
{
    Init();
}

UtilProcessLock::~UtilProcessLock()
{
#ifdef _WIN32 // Windows：Mutex 句柄会在进程退出时由系统自动回收
#elif __linux__ // Linux：手动删除 System V 信号量集
    semctl(semid, 0, IPC_RMID); // 从内核中移除信号量集
#endif
}

void UtilProcessLock::Lock()
{
#ifdef _WIN32
    DWORD dwRet = WaitForSingleObject(hLock, INFINITE); // 阻塞等待直到获取到互斥体所有权
#elif __linux__
    struct sembuf lock; // 信号量操作结构体
    lock.sem_num = 0;   // 操作第 0 个信号量
    lock.sem_op = -1;   // P 操作：信号量值减 1（若当前为 0 则阻塞）
    lock.sem_flg = SEM_UNDO; // 进程退出时自动撤销此操作
    semop(semid, &lock, 1); // 执行 1 个信号量操作
#endif
}

void UtilProcessLock::UnLock()
{
#ifdef _WIN32
    if (hLock) // 确保互斥体句柄有效
    {
        ReleaseMutex(hLock); // 释放互斥体所有权
    }
#elif __linux__
    struct sembuf unlock; // 信号量操作结构体
    unlock.sem_num = 0;   // 操作第 0 个信号量
    unlock.sem_op = 1;    // V 操作：信号量值加 1
    unlock.sem_flg = SEM_UNDO; // 进程退出时自动撤销
    semop(semid, &unlock, 1); // 执行 1 个信号量操作
#endif
}

void UtilProcessLock::Init()
{
#ifdef _WIN32
    int size = MultiByteToWideChar(CP_UTF8, 0, Name.c_str(), -1, NULL, 0); // 第一次调用计算所需宽字符缓冲区大小
    wchar_t* buffer = new wchar_t[size]; // 分配宽字符缓冲区
    MultiByteToWideChar(CP_UTF8, 0, Name.c_str(), -1, buffer, size); // 将 UTF-8 名称转换为宽字符串
    hLock = CreateMutexW(NULL, 0, buffer); // 创建或打开命名互斥体（NULL=默认安全描述符，0=不立即拥有）
    if (hLock == NULL) // 创建失败
    {
        printf("ERROR: UtilProcessLock: hLock is NULL"); // 输出错误信息
    }
    else
        printf("UtilProcessLock Success"); // 创建成功
#elif __linux__
    int key = std::hash<std::string>()(Name); // 使用 std::hash 将锁名称转换为 System V 信号量 key 值
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666); // 尝试独占创建信号量集（含 1 个信号量）
    if (semid < 0) // 信号量已存在或创建失败
    {
        printf("UtilProcessLock %s is Exists",Name.c_str()); // 日志提示锁已存在
        semid = semget(key, 1, IPC_CREAT | 0666); // 打开已存在的信号量集（不要求独占）
    }
    printf("semid: %d", semid); // 输出信号量 ID
    // 初始化信号量值为 1（二进制信号量/互斥锁）
    union semun arg;
    arg.val = 1; // 设置初始值为 1
    if (semctl(semid, 0, SETVAL, arg) < 0) // 对第 0 个信号量设置值
    {
        printf("semctl ERROR!"); // 设置失败
    }
#endif
}