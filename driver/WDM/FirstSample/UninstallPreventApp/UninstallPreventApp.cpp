// UninstallPreventApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include "../UninstallPrevent/UninstallPreventCommon.h"

int Error(const char* message) {
    printf("%s (error=%d)\n", message, GetLastError());
    return 1;
}

int main()
{
    std::cout << "Please input the pid of would you want to resume:" << std::endl;
    // 获取进程PID
    ULONG pid;
    std::cin >> pid;

    // 获取进程对象
    // 打开符号链接
    HANDLE hDevice = CreateFile(L"\\\\.\\UninstallPrevent", GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return Error("Failed to open device.");
    }

    DWORD returnVal;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_UNINSTALL_RESUME_PROCESS,
        &pid, sizeof(pid),
        nullptr, 0,
        &returnVal, nullptr);

    if (success) {
        printf("Reusme success.\n");
    }
    else {
        printf("Reusme fail.\n");
    }

    return 0;
}
