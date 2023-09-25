// SuspendResumeDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <cstdio>
#include <Windows.h>

void test() {
    while (true)
    {
        printf("test\n");
        Sleep(1000);
    }
}

int main()
{
    DebugBreak();
    auto handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test, NULL, 0, NULL);

    if (handle == NULL) {
        printf("CreateThread failed (%d)\n", GetLastError());
        return 1;
    }
    
    Sleep(3000); // 让线程运行 3 秒

    if (SuspendThread(handle) == -1) {
        printf("SuspendThread failed (%d)\n", GetLastError());
        return 1;
    }

    printf("SuspendThread success\n");

    Sleep(3000); // 挂起 3 秒

    if (ResumeThread(handle) == -1) {
        printf("ResumeThread failed (%d)\n", GetLastError());
        return 1;
    }

    printf("ResumeThread success\n");

    CloseHandle(handle);

    return 0;
}
