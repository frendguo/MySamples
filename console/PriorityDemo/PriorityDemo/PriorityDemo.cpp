// PriorityDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <phnt_windows.h>
#include <phnt.h>

enum ProcessMemoryPriorityEnum {
    Lowest = MEMORY_PRIORITY_LOWEST,
    VeryLow = MEMORY_PRIORITY_VERY_LOW,
    Low = MEMORY_PRIORITY_LOW,
    Medium = MEMORY_PRIORITY_MEDIUM,
    BelowNormal = MEMORY_PRIORITY_BELOW_NORMAL,
    Normal = MEMORY_PRIORITY_NORMAL
};

// 调整内存优先级
bool SetProcessMemoryPriority(DWORD processId, ProcessMemoryPriorityEnum priority)
{
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    MEMORY_PRIORITY_INFORMATION memoryPriorityInfo = {0};
    memoryPriorityInfo.MemoryPriority = (ULONG)priority;
    BOOL isSuccess = SetProcessInformation(hProcess, ProcessMemoryPriority, &memoryPriorityInfo, sizeof(MEMORY_PRIORITY_INFORMATION));
    if (!isSuccess) {
        std::cerr << "Failed to set process information. Error: " << GetLastError() << std::endl;
    }

    CloseHandle(hProcess);

    return isSuccess;
}

// 获取进程内存优先级
bool GetProcessMemoryPriority(DWORD processId, ProcessMemoryPriorityEnum& priority)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    MEMORY_PRIORITY_INFORMATION memoryPriorityInfo;
    if (!GetProcessInformation(hProcess, ProcessMemoryPriority, &memoryPriorityInfo, sizeof(memoryPriorityInfo))) {
        std::cerr << "Failed to get process information. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    priority = (ProcessMemoryPriorityEnum)memoryPriorityInfo.MemoryPriority;
    return TRUE;
}

int main()
{
    DWORD processId = GetCurrentProcessId();
    ProcessMemoryPriorityEnum priority;
    if (GetProcessMemoryPriority(processId, priority)) {
        std::cout << "Current process memory priority: " << priority << std::endl;
    }
    else {
        std::cerr << "Failed to set process memory priority. Error: " << GetLastError() << std::endl;
    }

    if (SetProcessMemoryPriority(processId, ProcessMemoryPriorityEnum::BelowNormal)) {
        std::cout << "Set process memory priority to Lowest." << std::endl;
    }

    if (GetProcessMemoryPriority(processId, priority)) {
        std::cout << "Current process memory priority: " << priority << std::endl;
    }
    else {
        std::cerr << "Failed to set process memory priority. Error: " << GetLastError() << std::endl;
    }
    return 0;
}