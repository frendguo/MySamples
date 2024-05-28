// SuspendProcessDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define PHNT_VERSION PHNT_WIN11

#include <iostream>
#include <phnt_windows.h>
#include <phnt.h>
#include <tlhelp32.h>
#include <conio.h>
#include <psapi.h>
#include <tchar.h>

#include "ProcessInfo.h"

#pragma comment(lib, "Psapi.lib")

HANDLE hDebugObject = nullptr;
HANDLE hJob = nullptr;
HANDLE hProcessStateChange = nullptr;
ULONG SuspendCount = 0;

#pragma region NtSuspendProcess

void ByNtSuspendProcess(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    if (NtSuspendProcess(hProcess) != STATUS_SUCCESS)
    {
        std::cout << "NtSuspendProcess failed" << std::endl;
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Process suspended" << std::endl;
    CloseHandle(hProcess);
}

void ByNtResumeProcess(DWORD dwProcessId) {
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    if (NtResumeProcess(hProcess) != STATUS_SUCCESS)
    {
        std::cout << "NtResumeProcess failed" << std::endl;
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Process resumed" << std::endl;
    CloseHandle(hProcess);
}

#pragma endregion

#pragma region Snapshot & SuspendThread

void BySnapshotAndSuspendThread(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateToolhelp32Snapshot failed" << std::endl;
        CloseHandle(hProcess);
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hSnapshot, &te32))
    {
        std::cout << "Thread32First failed" << std::endl;
        CloseHandle(hSnapshot);
        CloseHandle(hProcess);
        return;
    }

    do
    {
        if (te32.th32OwnerProcessID == dwProcessId)
        {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
            if (hThread == NULL)
            {
                std::cout << "OpenThread failed" << std::endl;
                CloseHandle(hSnapshot);
                CloseHandle(hProcess);
                return;
            }

            if (SuspendThread(hThread) == -1)
            {
                std::cout << "SuspendThread failed" << std::endl;
                CloseHandle(hThread);
                CloseHandle(hSnapshot);
                CloseHandle(hProcess);
                return;
            }

            std::cout << "Thread " << te32.th32ThreadID << " suspended" << std::endl;
            CloseHandle(hThread);
        }
    } while (Thread32Next(hSnapshot, &te32));

    CloseHandle(hSnapshot);
    CloseHandle(hProcess);
}

void BySnapshotAndResumeThread(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateToolhelp32Snapshot failed" << std::endl;
        CloseHandle(hProcess);
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hSnapshot, &te32))
    {
        std::cout << "Thread32First failed" << std::endl;
        CloseHandle(hSnapshot);
        CloseHandle(hProcess);
        return;
    }

    do
    {
        if (te32.th32OwnerProcessID == dwProcessId)
        {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
            if (hThread == NULL)
            {
                std::cout << "OpenThread failed" << std::endl;
                CloseHandle(hSnapshot);
                CloseHandle(hProcess);
                return;
            }

            if (ResumeThread(hThread) == -1)
            {
                std::cout << "ResumeThread failed" << std::endl;
                CloseHandle(hThread);
                CloseHandle(hSnapshot);
                CloseHandle(hProcess);
                return;
            }

            std::cout << "Thread " << te32.th32ThreadID << " resumed" << std::endl;
            CloseHandle(hThread);
        }
    } while (Thread32Next(hSnapshot, &te32));

    CloseHandle(hSnapshot);
    CloseHandle(hProcess);
}

#pragma endregion

#pragma region NtGetNextThread & SuspendThread

void ByNtGetNextThreadAndSuspendThread(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    HANDLE hThread = NULL;
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status = STATUS_SUCCESS;
    while (NtGetNextThread(hProcess, hThread, THREAD_SUSPEND_RESUME, 0, 0, &hThread) == STATUS_SUCCESS)
    {
        if (SuspendThread(hThread) == -1)
        {
            std::cout << "NtSuspendThread failed, error code: " << GetLastError() << std::endl;
        }
    }

    CloseHandle(hProcess);
}

void ByNtGetNextThreadAndResumeThread(DWORD dwProcessId) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    HANDLE hThread = nullptr;
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status = STATUS_SUCCESS;
    while (NtGetNextThread(hProcess, hThread, THREAD_SUSPEND_RESUME, 0, 0, &hThread) == STATUS_SUCCESS)
    {
        if (ResumeThread(hThread) == -1)
        {
            std::cout << "NtResumeThread failed, error code: " << GetLastError() << std::endl;
            break;
        }
    }

    CloseHandle(hProcess);
}

#pragma endregion

#pragma region NtCreateDebugObject

HANDLE ByNtCreateDebugObjectSuspendProcess(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return nullptr;
    }

    HANDLE hDebugObject = nullptr;
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);
    if (NtCreateDebugObject(&hDebugObject, DEBUG_ALL_ACCESS, &objAttr, 0) != STATUS_SUCCESS)
    {
        std::cout << "NtCreateDebugObject failed" << std::endl;
        CloseHandle(hProcess);
        hProcess = nullptr;
        return nullptr;
    }

    if (NtDebugActiveProcess(hProcess, hDebugObject) != STATUS_SUCCESS)
    {
        std::cout << "NtDebugActiveProcess failed" << std::endl;
        CloseHandle(hDebugObject);
        CloseHandle(hProcess);
        return nullptr;
    }

    std::cout << "Process suspended" << std::endl;
    CloseHandle(hProcess);
    return hDebugObject;
}

void ByNtRemoveProcessDebugResumeProcess(DWORD dwProcessId, HANDLE hDebugObject)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    if (NtRemoveProcessDebug(hProcess, hDebugObject) != STATUS_SUCCESS)
    {
        std::cout << "NtRemoveProcessDebug failed" << std::endl;
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Process resumed" << std::endl;
    CloseHandle(hProcess);
}

#pragma endregion

#pragma region JobObjectFreeze

#define JOB_OBJECT_OPERATION_FREEZE 1

HANDLE ByJobObjectFreeze(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return nullptr;
    }

    HANDLE hJob = CreateJobObject(NULL, NULL);
    if (hJob == NULL)
    {
        std::cout << "CreateJobObject failed" << std::endl;
        CloseHandle(hProcess);
        return nullptr;
    }

    JOBOBJECT_FREEZE_INFORMATION jfi;
    jfi.Flags = JOB_OBJECT_OPERATION_FREEZE;
    jfi.Freeze = true;

    // (JOBOBJECTINFOCLASS)18 <-> JobObjectFreezeInformation
    if (!NT_SUCCESS(NtSetInformationJobObject(hJob, (JOBOBJECTINFOCLASS)18, &jfi, sizeof(jfi))))
    {
        std::cout << "SetInformationJobObject failed" << std::endl;
        CloseHandle(hJob);
        CloseHandle(hProcess);
        return nullptr;
    }

    if (!AssignProcessToJobObject(hJob, hProcess))
    {
        std::cout << "AssignProcessToJobObject failed" << std::endl;
        CloseHandle(hJob);
        CloseHandle(hProcess);
        return nullptr;
    }

    std::cout << "Process freeze" << std::endl;
    CloseHandle(hProcess);

    return hJob;
}

void ByJobObjectUnFreeze(DWORD dwProcessId, HANDLE hObj) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    JOBOBJECT_FREEZE_INFORMATION jfi;
    jfi.Flags = JOB_OBJECT_OPERATION_FREEZE;
    jfi.Freeze = false;

    // (JOBOBJECTINFOCLASS)18 <-> JobObjectFreezeInformation
    if (!NT_SUCCESS(NtSetInformationJobObject(hObj, (JOBOBJECTINFOCLASS)18, &jfi, sizeof(jfi))))
    {
        std::cout << "SetInformationJobObject failed" << std::endl;
        CloseHandle(hObj);
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Process unfreeze" << std::endl;
    CloseHandle(hProcess);
}

#pragma endregion

#pragma region NtCreateProcessStateChange

#define PROCESS_STATE_CHANGE_STATE (0x0001) 
#define PROCESS_STATE_ALL_ACCESS STANDARD_RIGHTS_ALL | PROCESS_STATE_CHANGE_STATE

// 此方法只在 win11 以后的版本才有
HANDLE ByProcessStateChangeFreeze(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return nullptr;
    }

    HANDLE hProcessStateChange = nullptr;
    NTSTATUS status = NtCreateProcessStateChange(&hProcessStateChange, PROCESS_SET_INFORMATION | PROCESS_STATE_ALL_ACCESS, nullptr, hProcess, NULL);
    if (!NT_SUCCESS(status)) {
        std::cout << "NtCreateProcessStateChange failed, status is : " << status << std::endl;
        CloseHandle(hProcess);
        return nullptr;
    }

    status = NtChangeProcessState(hProcessStateChange, hProcess, PROCESS_STATE_CHANGE_TYPE::ProcessStateChangeSuspend, NULL, 0, 0);
    if (!NT_SUCCESS(status)) {
        std::cout << "NtChangeProcessState failed, status is : " << status << std::endl;
        CloseHandle(hProcessStateChange);
        CloseHandle(hProcess);
        return nullptr;
    }

    std::cout << "Process freeze" << std::endl;
    CloseHandle(hProcess);
}

void ByProcessStateChangeResume(DWORD processId, HANDLE hProcessStateChange) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return;
    }

    NTSTATUS status = NtChangeProcessState(hProcessStateChange, hProcess, PROCESS_STATE_CHANGE_TYPE::ProcessStateChangeResume, NULL, 0, 0);
    if (!NT_SUCCESS(status)) {
        std::cout << "NtChangeProcessState failed, status is: " << status << std::endl;
        CloseHandle(hProcessStateChange);
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Process unfreeze" << std::endl;
    CloseHandle(hProcessStateChange);
    CloseHandle(hProcess);
}

#pragma endregion

void ExecuteSuspendProcess(DWORD dwProcessId, int methodIndex)
{
    switch (methodIndex)
    {
    case 1:
    {
        ByNtSuspendProcess(dwProcessId);
        break;
    }
    case 2:
    {
        BySnapshotAndSuspendThread(dwProcessId);
        break;
    }
    case 3:
    {
        ByNtGetNextThreadAndSuspendThread(dwProcessId);
        break;
    }
    case 4: {
        hDebugObject = ByNtCreateDebugObjectSuspendProcess(dwProcessId);
        if (hDebugObject == nullptr)
        {
            break;
        }
        break;
    }
    case 5: {
        hJob = ByJobObjectFreeze(dwProcessId);
        if (hJob == nullptr)
        {
            break;
        }
        break;
    }
    case 6:
    {
        hProcessStateChange = ByProcessStateChangeFreeze(dwProcessId);
        if (hDebugObject == nullptr)
        {
            break;
        }
        break;
    }
    default:
        break;
    }
}

void ExecuteReusmeProcess(DWORD dwProcessId, int methodIndex) {
    switch (methodIndex)
    {
    case 1:
    {
        ByNtResumeProcess(dwProcessId);

        break;
    }
    case 2:
    {
        BySnapshotAndResumeThread(dwProcessId);

        break;
    }
    case 3:
    {
        ByNtGetNextThreadAndResumeThread(dwProcessId);

        break;
    }
    case 4: {
        if (hDebugObject == nullptr)
        {
            std::cout << "Debug object is null. Please suspend process first." << std::endl;
            break;
        }
        ByNtRemoveProcessDebugResumeProcess(dwProcessId, hDebugObject);
        CloseHandle(hDebugObject);
        hDebugObject = nullptr;

        break;
    }
    case 5: {
        if (hJob == nullptr)
        {
            std::cout << "Job object is null. Please suspend process first." << std::endl;
            break;
        }
        ByJobObjectUnFreeze(dwProcessId, hJob);
        CloseHandle(hJob);
        hJob = nullptr;

        break;
    }
    case 6:
    {
        if (hProcessStateChange == nullptr)
        {
            std::cout << "Process state change object is null. Please suspend process first." << std::endl;
            break;
        }
        ByProcessStateChangeResume(dwProcessId, hProcessStateChange);
        break;
    }
    default:
        break;
    }
}

bool IsSystemProcess(DWORD processID) {
    TCHAR processPath[MAX_PATH] = TEXT("");

    if (processID < 1000) {
        return true;
    }

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        // Get the full path of the executable file for the process.
        if (GetModuleFileNameEx(hProcess, NULL, processPath, MAX_PATH)) {
            // Check if the path starts with "C:\Windows\System32"
            if (_tcsncmp(processPath, TEXT("C:\\Windows\\System32\\"), 20) == 0) {
                CloseHandle(hProcess);
                return true;
            }

            // Check if the path starts with "C:\Windows\SystemApps"
            if (_tcsncmp(processPath, TEXT("C:\\Windows\\SystemApps\\"), 21) == 0) {
                CloseHandle(hProcess);
                return true;
            }
        }
        CloseHandle(hProcess);
    }
    else {
        // 无法打开这个进程，可能是因为权限不足
        return true;
    }
    return false;
}

void GetThreadsByProcessHandle(HANDLE hProcess, std::vector<ThreadInfo>& threads)
{
    HANDLE hThread = NULL;
    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status = STATUS_SUCCESS;
    while (NtGetNextThread(hProcess, hThread, THREAD_QUERY_LIMITED_INFORMATION | THREAD_TERMINATE, 0, 0, &hThread) == STATUS_SUCCESS)
    {
        ThreadInfo threadInfo;
        threadInfo.ThreadId = GetThreadId(hThread);

        /*UNICODE_STRING threadName;
        status = NtQueryInformationThread(hThread, ThreadNameInformation, &threadName, sizeof(threadName), NULL);
        if (!NT_SUCCESS(status))
        {
            std::cout << "QueryInformationThread failed" << std::endl;
            break;
        }*/

        ULONG isTerminated = 0;
        ULONG retLen;
        status = NtQueryInformationThread(hThread, ThreadIsTerminated, &isTerminated, sizeof(isTerminated), &retLen);
        if (!NT_SUCCESS(status))
        {
            std::cout << "[ThreadIsTerminated]QueryInformationThread failed, thread id: " << threadInfo.ThreadId << "status: " << status << std::endl;
            continue;
        }

        if (isTerminated)
        {
            continue;
        }

        int suspendCount = 0;
        status = NtQueryInformationThread(hThread, ThreadSuspendCount, &suspendCount, sizeof(threadInfo.SuspendCount), NULL);
        if (!NT_SUCCESS(status))
        {
            std::cout << "[ThreadSuspendCount]QueryInformationThread failed, thread id: " << threadInfo.ThreadId << "status: " << status << std::endl;
            continue;
        }
        threadInfo.SuspendCount = suspendCount;

        threads.push_back(threadInfo);
    }
}

ProcessInfo GetProcessInfo(DWORD processId)
{
    ProcessInfo processInfo;
    processInfo.ProcessId = processId;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed" << std::endl;
        return processInfo;
    }

    TCHAR imageName[MAX_PATH] = TEXT("");
    if (GetProcessImageFileName(hProcess, imageName, MAX_PATH))
    {
        processInfo.ProcessName = imageName;
    }

    PROCESS_EXTENDED_BASIC_INFORMATION pebi;

    NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pebi, sizeof(pebi), NULL);
    if (!NT_SUCCESS(status))
    {
        std::cout << "QueryInformationProcess failed" << std::endl;
        return processInfo;
    }

    processInfo.IsFreeze = pebi.IsFrozen;
    GetThreadsByProcessHandle(hProcess, processInfo.Threads);

    CloseHandle(hProcess);
    return processInfo;
}

bool IsProcessSuspend(DWORD processId) {
    ProcessInfo processInfo = GetProcessInfo(processId);
    bool isSuspend = processInfo.IsFreeze;
    std::cout << "Process id: " << processInfo.ProcessId << ", Is freeze: " << processInfo.IsFreeze << std::endl;
    if (processInfo.Threads.size() > 0) {
        isSuspend = true;
    }
    for (auto& thread : processInfo.Threads) {
        std::cout << "Thread id: " << thread.ThreadId << ", Suspend count: " << thread.SuspendCount << std::endl;
        if (thread.SuspendCount < 1) {
            isSuspend = false;
            break;
        }
    }

    return isSuspend;
}

void ExecuteStressTesting(int methodIndex)
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot (of processes) failed: " << GetLastError() << std::endl;
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process
    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "Process32First failed: " << GetLastError() << std::endl; // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return;
    }

    do {
        // 过滤掉系统进程
        if (IsSystemProcess(pe32.th32ProcessID)) {
            std::cout << "Skip system process: " << pe32.th32ProcessID << "-" << std::endl;
            continue;
        }

        // 过滤掉自身
        auto selfId = GetCurrentProcessId();
        if (pe32.th32ProcessID == selfId) {
            std::cout << "Skip self process: " << pe32.th32ProcessID << "-" << std::endl;
            continue;
        }

        // 如果是挂起的进程，跳过
        if (IsProcessSuspend(pe32.th32ProcessID)) {
            std::cout << "Skip suspend process: " << pe32.th32ProcessID << "-" << std::endl;
            continue;
        }

        // 如果进程的线程数小于0，跳过
        if (GetProcessInfo(pe32.th32ProcessID).Threads.size() < 0) {
            std::cout << "Skip process with no threads: " << pe32.th32ProcessID << "-" << std::endl;
            continue;
        }

        // 执行挂起操作
        ExecuteSuspendProcess(pe32.th32ProcessID, methodIndex);

        // 验证挂起操作是否成功
        if (!IsProcessSuspend(pe32.th32ProcessID)) {
            std::cerr << "[Suspend]Stress testing failed on process: " << pe32.th32ProcessID << std::endl;
            std::cout << "----Suspend count: " << SuspendCount << std::endl;
            system("pause");
        }
        SuspendCount++;

        std::cout << "Process suspend success, will resume process. process:" << pe32.th32ProcessID << std::endl;

        // 执行恢复操作
        ExecuteReusmeProcess(pe32.th32ProcessID, methodIndex);

        // 验证恢复操作是否成功
        if (IsProcessSuspend(pe32.th32ProcessID)) {
            std::cerr << "[Resume]Stress testing failed on process: " << pe32.th32ProcessID << std::endl;
            system("pause");
        }
    } while (Process32Next(hProcessSnap, &pe32));
}

int main()
{
    /*auto info = GetProcessInfo(9512);

    std::cout << "Process id: " << info.ProcessId << std::endl;
    std::wcout << "Process name: " << info.ProcessName << std::endl;
    std::cout << "Is freeze: " << info.IsFreeze << std::endl;

    for (auto thread : info.Threads)
    {
        std::cout << "-------------------------" << std::endl;
        std::cout << "Thread id: " << thread.ThreadId << std::endl;
        std::cout << "Suspend count: " << thread.SuspendCount << std::endl;
        std::cout << "-------------------------" << std::endl;
    }*/

    while (true)
    {
        DWORD dwProcessId = 0;
        int methodIndex;
        int modeIndex;
        system("cls");

        // 测试模式选择
        std::cout << "Please chose the test mode:" << std::endl;
        std::cout << "1. Test by process id" << std::endl;
        std::cout << "2. Automate Stress Testing" << std::endl;
        std::cin >> modeIndex;

        if (modeIndex == 1) {
            std::cout << "Enter process id: ";
            std::cin >> dwProcessId;
        }

        system("cls");
        std::cout << "Please chose the method to suspend the process:" << std::endl;
        std::cout << "1. By NtSuspendProcess" << std::endl;
        std::cout << "2. By Snapshot and SuspendThread" << std::endl;
        std::cout << "3. By NtGetNextThread and SuspendThread" << std::endl;
        std::cout << "4. By Debug Object Suspend Process" << std::endl;
        std::cout << "5. By Job Object Freeze" << std::endl;
        std::cout << "6. By Process State Change Freeze" << std::endl;
        std::cin >> methodIndex;

        switch (modeIndex)
        {
        case 1:
        {
            // 执行一个进程的挂起操作
            if (dwProcessId == 0)
            {
                std::cout << "Invalid process id" << std::endl;
                break;
            }
            ExecuteSuspendProcess(dwProcessId, methodIndex);
            std::cout << "Process suspended, Press any key to resume." << std::endl;
            system("pause");
            ExecuteReusmeProcess(dwProcessId, methodIndex);
            break;
        }
        case 2:
        {
            while (true)
            {
                // 自动化压力测试
                ExecuteStressTesting(methodIndex);

                std::cout << "---------ready start next streess test.------------" << std::endl;
                std::cout << "----Suspend count: " << SuspendCount << std::endl;
                Sleep(1000);
            }
            
            break;
        }
        default:
            break;
        }
    }

    return 0;
}