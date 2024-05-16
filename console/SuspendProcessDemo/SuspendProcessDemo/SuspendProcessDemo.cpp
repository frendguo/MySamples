// SuspendProcessDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <phnt_windows.h>
#include <phnt.h>
#include <tlhelp32.h>
#include <conio.h>

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

int main()
{
    while (true)
    {
        DWORD dwProcessId = 0;
        int methodIndex;
        system("cls");
        std::cout << "Enter process id: ";
        std::cin >> dwProcessId;

        system("cls");
        std::cout << "Please chose the method to suspend the process:" << std::endl;
        std::cout << "1. By NtSuspendProcess" << std::endl;
        std::cout << "2. By Snapshot and SuspendThread" << std::endl;
        std::cout << "3. By NtGetNextThread and SuspendThread" << std::endl;
        std::cout << "4. By Debug Object Suspend Process" << std::endl;
        std::cout << "5. By Job Object Freeze" << std::endl;
        std::cin >> methodIndex;

        switch (methodIndex)
        {
        case 1:
        {
            ByNtSuspendProcess(dwProcessId);
            std::cout << "Press any key to resume the process" << std::endl;
            _getch();
            ByNtResumeProcess(dwProcessId);

            break;
        }
        case 2:
        {
            BySnapshotAndSuspendThread(dwProcessId);
            std::cout << "Press any key to resume the process" << std::endl;
            _getch();
            BySnapshotAndResumeThread(dwProcessId);

            break;
        }
        case 3:
        {
            ByNtGetNextThreadAndSuspendThread(dwProcessId);
            std::cout << "Press any key to resume the process" << std::endl;
            _getch();
            ByNtGetNextThreadAndResumeThread(dwProcessId);

            break;
        }
        case 4: {
            HANDLE hDebugObject = ByNtCreateDebugObjectSuspendProcess(dwProcessId);
            if (hDebugObject == nullptr)
            {
                break;
            }
            std::cout << "Press any key to resume the process" << std::endl;
            _getch();
            ByNtRemoveProcessDebugResumeProcess(dwProcessId, hDebugObject);
            CloseHandle(hDebugObject);

            break;
        }
        case 5: {
            HANDLE hJob = ByJobObjectFreeze(dwProcessId);
            if (hJob == nullptr)
            {
                break;
            }
            std::cout << "Press any key to resume the process" << std::endl;
            _getch();
            ByJobObjectUnFreeze(dwProcessId, hJob);
            CloseHandle(hJob);

            break;
        }
        default:
            break;
        }
    }

    return 0;
}