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
    MEMORY_COMBINE_INFORMATION_EX combineInfo = {0};
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

int main()
{
    NTSTATUS status = ERROR_SUCCESS;

    EnablePrivilege();

    // 1. 清理系统文件缓存
    status = CleanSystemFileCache();
    if (status == 0) {
        printf("CleanSystemFileCache success\n");
    }
    else {
        // 如果失败，打印提示，并输出 status
        printf("CleanSystemFileCache failed, status: 0x%08X\n", status);
    }

    // 2. 清空进程工作集
    status = SystemEmptyWorkingSet();
    if (status == 0) {
        printf("SystemEmptyWorkingSet success\n");
    }
    else {
        printf("SystemEmptyWorkingSet failed, status: 0x%08X\n", status);
    }

    // 3. 清理低优先级的等待列表(standby list)
    status = PurgeLowPriorityStandbyList();
    if (status == 0) {
        printf("PurgeLowPriorityStandbyList success\n");
    }
    else {
        printf("PurgeLowPriorityStandbyList failed, status: 0x%08X\n", status);
    }

    // 4. 清理内存中的等待列表(standby list)
    status = PurgeStandbyList();
    if (status == 0) {
        printf("PurgeStandbyList success\n");
    }
    else {
        printf("PurgeStandbyList failed, status: 0x%08X\n", status);
    }

    // 5. 整理内存修改列表（modify list）
    status = FlushModifyedList();
    if (status == 0) {
        printf("FlushModifyedList success\n");
    }
    else {
        printf("FlushModifyedList failed, status: 0x%08X\n", status);
    }

    // 6. 将注册表 flush 到磁盘
    status = FlushRegistry();
    if (status == 0) {
        printf("FlushRegistry success\n");
    }
    else {
        printf("FlushRegistry failed, status: 0x%08X\n", status);
    }

    // 7. 组合多个物理页面
    status = CombinePhysicalMemory();
    if (status == 0) {
        printf("CombinePhysicalMemory success\n");
    }
    else {
        printf("CombinePhysicalMemory failed, status: 0x%08X\n", status);
    }
    
    system("pause");

    return 0;
}
