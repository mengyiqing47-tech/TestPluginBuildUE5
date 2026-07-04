#pragma once
#include <iostream>

class AGLOBALUTIL_API UtilProcessLock
{
public:
    UtilProcessLock();
    UtilProcessLock(std::string name);
    ~UtilProcessLock();
    
    void Lock();
    void UnLock();
private:
    void Init();
private:
    std::string Name;

#ifdef _WIN32
    void* hLock = NULL;
#elif __linux__
    int semid = -1;
#endif
};
