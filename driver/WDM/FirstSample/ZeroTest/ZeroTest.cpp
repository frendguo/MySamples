// ZeroTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <errhandlingapi.h>
#include <fileapi.h>

int Error(const char* msg) {
    printf("s%: Error=d%\n", msg, GetLastError());
    return 1;
}

int main()
{
    HANDLE hDevice = CreateFile(L"\\\\.\\Zero", GENERIC_READ | GENERIC_WRITE, 
        0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hDevice == INVALID_HANDLE_VALUE) {
        return Error("failed to open device.");
    }

    printf("------read test-----\n");

    byte buffer[64]{};
    for (int i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = i + 1;
    }

    DWORD bytes = 0;
    bool res = ReadFile(hDevice, buffer, sizeof(buffer), &bytes, nullptr);
    if (!res) {
        return ERROR("failed to read file.");
    }

    if (sizeof(buffer) != bytes) {
        printf("Wrong number of bytes!\n");
    }

    long total = 0;
    for (auto n : buffer) {
        total += n;
    }

    if (total != 0) {
        printf("Wrong data!\n");
    }

    printf("------write test-----\n");

    BYTE buffer2[1024]{};
    res = WriteFile(hDevice, buffer2, sizeof(buffer2), &bytes, nullptr);
    if (!res) {
        return ERROR("failed to write file.");
    }

    if (bytes != sizeof(buffer2)) {
        printf("Wrong byte count.\n");
    }

    CloseHandle(hDevice);
}
