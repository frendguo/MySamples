// EnumProcessDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <phnt_windows.h>
#include <phnt.h>
#include <vector>
#include <tlhelp32.h>
#include <chrono>
#include <psapi.h>

void EnumProcess(std::vector<DWORD>& pids) {
    HANDLE hProcess = nullptr;
    HANDLE nextProcess = NULL;
    while (NT_SUCCESS(NtGetNextProcess(hProcess, PROCESS_QUERY_INFORMATION, 0, 0, &nextProcess))) {
        auto pid = GetProcessId(nextProcess);
        pids.push_back(pid);
        hProcess = nextProcess;
    }
}

void EnumProcessBySnap(std::vector<DWORD>& pids) {
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

    do
    {
        pids.push_back(pe32.th32ProcessID);
    } while (Process32Next(hProcessSnap, &pe32));
}

void EnumProcessByQueryInformation(std::vector<DWORD>& pids) {
    DWORD   dwSize = 0xFFFFF;
    LPTSTR pBuf = new TCHAR[dwSize];
    PSYSTEM_PROCESS_INFORMATION pInfo = { 0 };

    pInfo = (PSYSTEM_PROCESS_INFORMATION)pBuf;
    auto status = NtQuerySystemInformation(SystemProcessInformation, pInfo, dwSize, &dwSize);
    if (!NT_SUCCESS(status)) {
        std::cerr << "NtQuerySystemInformation failed: " << status << std::endl;
        return;
    }

    while (pInfo->NextEntryOffset)
    {
        pids.push_back(HandleToULong(pInfo->UniqueProcessId));

        pInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pInfo) + pInfo->NextEntryOffset);
    }
}

int main()
{
    std::vector<DWORD> pids;
    std::vector<DWORD> pids2;
    std::vector<DWORD> pids3;

    auto start = std::chrono::high_resolution_clock::now();

    DWORD aProcesses[1024] = { 0 }, cbNeeded, cProcesses;
    EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);
    
    auto end = std::chrono::high_resolution_clock::now();

    EnumProcessBySnap(pids);

    auto end2 = std::chrono::high_resolution_clock::now();

    EnumProcessByQueryInformation(pids2);

    auto end3 = std::chrono::high_resolution_clock::now();

    EnumProcess(pids3);

    auto end4 = std::chrono::high_resolution_clock::now();

    std::cout << "Method1(EnumProcesses) Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
    std::cout << "Method2(EnumProcessBySnap) Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end2 - end).count() << "us" << std::endl;
    std::cout << "Method3(EnumProcessByQueryInformation) Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end3 - end2).count() << "us" << std::endl;
    std::cout << "Method4(EnumProcessByNative) Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end4 - end3).count() << "us" << std::endl;

    system("pause");

    return 0;
}
