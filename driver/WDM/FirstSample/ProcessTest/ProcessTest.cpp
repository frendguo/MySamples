#include <stdio.h>
#include <Windows.h>
#include <string>

int main()
{
    auto hDevice = CreateFile(L"\\\\.\\ProcessInterceptor", GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to Create file. ErrorCode is %d", GetLastError());
        return 0;
    }

    std::string s = "InstDrv.exe";
    DWORD bytes = 0;
    
    WriteFile(hDevice, s.c_str(), s.size(), &bytes, nullptr);

    return 0;
}
