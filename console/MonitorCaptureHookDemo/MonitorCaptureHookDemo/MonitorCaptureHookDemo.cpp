#include <iostream>
#include <Windows.h>

int main()
{
    LPCSTR dllPath = "D:\\HookDll.dll";
    DWORD pid = 48240;
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
