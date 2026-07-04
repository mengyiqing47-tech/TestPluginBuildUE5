#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

struct UtilShareMemoryHandle;

class AGLOBALUTIL_API UtilShareMemory
{
public:
    UtilShareMemory();
    UtilShareMemory(std::string name);
    ~UtilShareMemory();

    std::string GetName();
    void SetName(std::string name);
    
	bool Create(unsigned int size);
    void* Open();
    void ResetHandle();

private:
    void Init();
    void Release();
	void UnMappingFileImp();
	void CloseHandleImp();
	void ResetParamImp();

private:
    std::string Name;
    UtilShareMemoryHandle *Handle;
};
