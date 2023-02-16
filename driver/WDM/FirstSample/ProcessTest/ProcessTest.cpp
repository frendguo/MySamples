#include <stdio.h>
#include <Windows.h>
#include <string>

int main()
{
    auto hDevice = CreateFile(L"\\??\\ProcessProtect", GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to Create file. ErrorCode is %d", GetLastError());
        return 0;
    }

    //std::string s = "InstDrv.exe";
    DWORD bytes = 0;
    ULONG pid = 4236;
    
    WriteFile(hDevice, &pid, sizeof(pid), &bytes, nullptr);

    return 0;
}
