// MemoryCleanDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <psapi.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

/// <summary>
/// 组合多个物理页面
/// 用于减少内存碎片化。提升物理内存利用率
/// </summary>
/// <returns></returns>
NTSTATUS CombinePhysicalMemory() {
    MEMORY_COMBINE_INFORMATION_EX combineInfo = { 0 };
    return NtSetSystemInformation(SystemCombinePhysicalMemoryInformation, &combineInfo, sizeof(combineInfo));
}

/// <summary>
/// 清理系统文件缓存
/// </summary>
/// <returns></returns>
NTSTATUS CleanSystemFileCache() {
    SYSTEM_FILECACHE_INFORMATION sfci = { 0 };

    sfci.MinimumWorkingSet = MAXSIZE_T;
    sfci.MaximumWorkingSet = MAXSIZE_T;
    return NtSetSystemInformation(SystemFileCacheInformation, &sfci, sizeof(sfci));
}

/// <summary>
/// 清空进程工作集
/// 这里清空并不是全部清空，还是尽可能的清理
/// </summary>
/// <returns></returns>
NTSTATUS SystemEmptyWorkingSet() {
    SYSTEM_MEMORY_LIST_COMMAND command = MemoryEmptyWorkingSets;

    return NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
}

/// <summary>
/// 清理低优先级的等待列表(standby list)
/// </summary>
/// <returns></returns>
NTSTATUS PurgeLowPriorityStandbyList() {
    SYSTEM_MEMORY_LIST_COMMAND command = MemoryPurgeLowPriorityStandbyList;

    return NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
}

/// <summary>
/// 清理内存中的等待列表(standby list)
/// </summary>
/// <returns></returns>
NTSTATUS PurgeStandbyList() {
    SYSTEM_MEMORY_LIST_COMMAND command = MemoryPurgeStandbyList;

    return NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
}

/// <summary>
/// 整理内存修改列表（modify list）
/// </summary>
/// <returns></returns>
NTSTATUS FlushModifyedList() {
    SYSTEM_MEMORY_LIST_COMMAND command = MemoryFlushModifiedList;

    return NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
}

/// <summary>
/// 将注册表 flush 到磁盘
/// </summary>
/// <returns></returns>
NTSTATUS FlushRegistry() {
    return NtSetSystemInformation(SystemRegistryReconciliationInformation, NULL, 0);
}

void Win32EmptyWorkingSet()
{
    HANDLE hProcess = GetCurrentProcess();
    EmptyWorkingSet(hProcess);
    CloseHandle(hProcess);
}

bool SetPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, BOOL bEnablePrivilege) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid)) {        // receives LUID of privilege
        std::cout << "LookupPrivilegeValue error: " << GetLastError() << std::endl;
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    // Enable the privilege or disable all privileges.
    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL)) {
        std::cout << "AdjustTokenPrivileges error: " << GetLastError() << std::endl;
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        std::cout << "The token does not have the specified privilege." << std::endl;
        return false;
    }

    return true;
}

bool EnablePrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cout << "OpenProcessToken error: " << GetLastError() << std::endl;
        return false;
    }

    bool bRet = SetPrivilege(hToken, SE_INCREASE_QUOTA_NAME, TRUE);
    if (!bRet) {
        std::cout << "SetPrivilege error: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    bRet = SetPrivilege(hToken, SE_PROF_SINGLE_PROCESS_NAME, TRUE);
    if (!bRet) {
        std::cout << "SetPrivilege error: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return bRet;
}

typedef struct _MEMORY_OBJECT
{
    ULONG64 total_bytes;
    ULONG64 free_bytes;
    ULONG64 used_bytes;
    ULONG percent;
} MEMORY_OBJECT, * PMEMORY_OBJECT;

typedef struct _MEMORY_INFO
{
    MEMORY_OBJECT physical_memory;
    MEMORY_OBJECT virtual_memory;
    MEMORY_OBJECT system_cache;
} MEMORY_INFO, * PMEMORY_INFO;

void GetMemoryInfo(PMEMORY_INFO memInfo) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    SYSTEM_FILECACHE_INFORMATION sfci = { 0 };

    if (GlobalMemoryStatusEx(&statex)) {
        // physical memory
        memInfo->physical_memory.total_bytes = statex.ullTotalPhys;
        memInfo->physical_memory.free_bytes = statex.ullAvailPhys;
        memInfo->physical_memory.used_bytes = statex.ullTotalPhys - statex.ullAvailPhys;
        memInfo->physical_memory.percent = (ULONG)(memInfo->physical_memory.used_bytes * 100 / memInfo->physical_memory.total_bytes);

        // virtual memory
        memInfo->virtual_memory.total_bytes = statex.ullTotalVirtual;
        memInfo->virtual_memory.free_bytes = statex.ullAvailVirtual;
        memInfo->virtual_memory.used_bytes = statex.ullTotalVirtual - statex.ullAvailVirtual;
    }

    // system cache information
    NTSTATUS status = NtQuerySystemInformation(SystemFileCacheInformation, &sfci, sizeof(sfci), NULL);
    if (NT_SUCCESS(status)) {
        memInfo->system_cache.total_bytes = sfci.CurrentSize;
        memInfo->system_cache.free_bytes = sfci.PeakSize;
        memInfo->system_cache.used_bytes = sfci.PeakSize - sfci.CurrentSize;
    }
}

void cleanMemory() {
    // 1. 清理系统文件缓存
    NTSTATUS status = CleanSystemFileCache();
    if (NT_SUCCESS(status)) {
        printf("CleanSystemFileCache success\n");
    }
    else {
        // 如果失败，打印提示，并输出 status
        printf("CleanSystemFileCache failed, status: 0x%08X\n", status);
    }

    // 2. 清空进程工作集
    status = SystemEmptyWorkingSet();
    if (NT_SUCCESS(status)) {
        printf("SystemEmptyWorkingSet success\n");
    }
    else {
        printf("SystemEmptyWorkingSet failed, status: 0x%08X\n", status);
    }

    // 3. 清理低优先级的等待列表(standby list)
    status = PurgeLowPriorityStandbyList();
    if (NT_SUCCESS(status)) {
        printf("PurgeLowPriorityStandbyList success\n");
    }
    else {
        printf("PurgeLowPriorityStandbyList failed, status: 0x%08X\n", status);
    }

    // 4. 清理内存中的等待列表(standby list)
    status = PurgeStandbyList();
    if (NT_SUCCESS(status)) {
        printf("PurgeStandbyList success\n");
    }
    else {
        printf("PurgeStandbyList failed, status: 0x%08X\n", status);
    }

    // 5. 整理内存修改列表（modify list）
    status = FlushModifyedList();
    if (NT_SUCCESS(status)) {
        printf("FlushModifyedList success\n");
    }
    else {
        printf("FlushModifyedList failed, status: 0x%08X\n", status);
    }

    // 6. 将注册表 flush 到磁盘
    status = FlushRegistry();
    if (NT_SUCCESS(status)) {
        printf("FlushRegistry success\n");
    }
    else {
        printf("FlushRegistry failed, status: 0x%08X\n", status);
    }

    // 7. 组合多个物理页面
    status = CombinePhysicalMemory();
    if (NT_SUCCESS(status)) {
        printf("CombinePhysicalMemory success\n");
    }
    else {
        printf("CombinePhysicalMemory failed, status: 0x%08X\n", status);
    }
}

int main()
{
    NTSTATUS status = ERROR_SUCCESS;

    EnablePrivilege();
    while (true)
    {



        std::cout << "Memory Clean Demo" << std::endl;
        std::cout << "Please input index to start..." << std::endl;
        std::cout << "1. CleanSystemFileCache" << std::endl;
        std::cout << "2. SystemEmptyWorkingSet" << std::endl;
        std::cout << "3. PurgeLowPriorityStandbyList" << std::endl;
        std::cout << "4. PurgeStandbyList" << std::endl;
        std::cout << "5. FlushModifyedList" << std::endl;
        std::cout << "6. FlushRegistry" << std::endl;
        std::cout << "7. CombinePhysicalMemory" << std::endl;
        std::cout << "8. All" << std::endl;

        int index = 0;
        std::cin >> index;
        // 获取内存信息
        MEMORY_INFO beforMemInfo = { 0 };
        GetMemoryInfo(&beforMemInfo);

        switch (index)
        {
        case 1:
            // 1. 清理系统文件缓存
            status = CleanSystemFileCache();
            if (NT_SUCCESS(status)) {
                printf("CleanSystemFileCache success\n");
            }
            else {
                // 如果失败，打印提示，并输出 status
                printf("CleanSystemFileCache failed, status: 0x%08X\n", status);
            }
            break;
        case 2:
            // 2. 清空进程工作集
            status = SystemEmptyWorkingSet();
            if (NT_SUCCESS(status)) {
                printf("SystemEmptyWorkingSet success\n");
            }
            else {
                printf("SystemEmptyWorkingSet failed, status: 0x%08X\n", status);
            }
            break;
        case 3:
            // 3. 清理低优先级的等待列表(standby list)
            status = PurgeLowPriorityStandbyList();
            if (NT_SUCCESS(status)) {
                printf("PurgeLowPriorityStandbyList success\n");
            }
            else {
                printf("PurgeLowPriorityStandbyList failed, status: 0x%08X\n", status);
            }
            break;

        case 4:
            // 4. 清理内存中的等待列表(standby list)
            status = PurgeStandbyList();
            if (NT_SUCCESS(status)) {
                printf("PurgeStandbyList success\n");
            }
            else {
                printf("PurgeStandbyList failed, status: 0x%08X\n", status);
            }
            break;
        case 5:
            // 5. 整理内存修改列表（modify list）
            status = FlushModifyedList();
            if (NT_SUCCESS(status)) {
                printf("FlushModifyedList success\n");
            }
            else {
                printf("FlushModifyedList failed, status: 0x%08X\n", status);
            }
            break;
        case 6:
            // 6. 将注册表 flush 到磁盘
            status = FlushRegistry();
            if (NT_SUCCESS(status)) {
                printf("FlushRegistry success\n");
            }
            else {
                printf("FlushRegistry failed, status: 0x%08X\n", status);
            }
            break;
        case 7:
            // 7. 组合多个物理页面
            status = CombinePhysicalMemory();
            if (NT_SUCCESS(status)) {
                printf("CombinePhysicalMemory success\n");
            }
            else {
                printf("CombinePhysicalMemory failed, status: 0x%08X\n", status);
            }
            break;
        case 8:
            cleanMemory();
            break;
        default:
            std::cout << "Invalid index" << std::endl;
            break;
        }

        printf("\n");
        printf("Sleep 2s...\n");
        Sleep(2 * 1000);

        // 获取内存信息
        MEMORY_INFO afterMemInfo = { 0 };
        GetMemoryInfo(&afterMemInfo);

        printf("Clear Result:\n");
        printf("Physical Memory:\n");
        std::cout << beforMemInfo.physical_memory.used_bytes / 1024.0 / 1024.0 << "MB -> " << afterMemInfo.physical_memory.used_bytes / 1024.0 / 1024.0 << "MB" << std::endl;
        std::cout << beforMemInfo.physical_memory.percent << "% -> " << afterMemInfo.physical_memory.percent << "%" << std::endl;
        printf("Virtual Memory:\n");
        std::cout << beforMemInfo.virtual_memory.used_bytes / 1024.0 / 1024.0 << "MB -> " << afterMemInfo.virtual_memory.used_bytes / 1024.0 / 1024.0 << "MB" << std::endl;
        printf("System Cache:\n");
        std::cout << beforMemInfo.system_cache.used_bytes / 1024.0 / 1024.0 << "MB -> " << afterMemInfo.system_cache.used_bytes / 1024.0 / 1024.0 << "MB" << std::endl;
        system("pause");

        // 清空输出
        system("cls");
    }
    
    return 0;
}
