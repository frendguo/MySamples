#include <iostream>
#include <vector>
#include <Windows.h>
#include <tlhelp32.h>

int InjectProcess(DWORD pid) {
    LPCSTR dllPath = "D:\\HookDll.dll";

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL)
    {
        std::cout << "OpenProcess failed--" << GetLastError() << std::endl;
        return 1;
    }

    LPVOID pDllPath = VirtualAllocEx(hProcess, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (pDllPath == NULL)
    {
        std::cout << "VirtualAllocEx failed--" << GetLastError() << std::endl;
        return 1;
    }

    // 写入DLL路径
    WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, 0);

    // 创建远程线程
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pDllPath, 0, 0);

    // 等待远程线程结束
    WaitForSingleObject(hThread, INFINITE);

    // 清理
    VirtualFreeEx(hProcess, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return 0;
}

std::vector<DWORD> FindProcessIdByName(LPCWSTR name) {
    std::vector<DWORD> pids;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateToolhelp32Snapshot failed--" << GetLastError() << std::endl;
        return pids;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe))
    {
        std::cout << "Process32First failed--" << GetLastError() << std::endl;
        CloseHandle(hSnapshot);
        return pids;
    }

    do
    {
        if (wcscmp(pe.szExeFile, name) == 0)
        {
            pids.push_back(pe.th32ProcessID);
        }
    } while (Process32Next(hSnapshot, &pe));

    CloseHandle(hSnapshot);
    return pids;
}

int main()
{
    auto pids = FindProcessIdByName(L"snipaste.exe");
    for (auto pid : pids) {
        std::cout << "Injecting process " << pid << std::endl;
        InjectProcess(pid);
    }

    return 0;
}
