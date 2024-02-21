#include <windows.h>
#include <dbghelp.h>
#include <iostream>

#include "KDmpStruct.h"
#include <wchar.h>

// 读取和分析minidump文件
void AnalyzeMinidump(LPCWSTR minidumpPath) {
    HANDLE hFile = CreateFile(minidumpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Cannot open minidump file. error code is " << GetLastError() << std::endl;
        return;
    }

    HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) {
        std::cout << "Cannot create file mapping. error code is " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return;
    }

    PVOID mapAddress = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (mapAddress == NULL) {
        std::cout << "Cannot map view of file. error code is " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return;
    }

    auto dumpHeader = (HEADER64*)mapAddress;
    if (!dumpHeader->LooksGood()) {
        printf("Invalid minidump file.\n");
        std::cout << "Invalid minidump file." << std::endl;
    }
    else {
        std::wcout << "Dump file path: " << minidumpPath << std::endl;
        std::cout << "DumpType : " << (int)dumpHeader->DumpType << std::endl;
        std::cout << "MajorVersion : " << dumpHeader->MajorVersion << std::endl;
        std::cout << "MinorVersion : " << dumpHeader->MinorVersion << std::endl;
        std::cout << "BugCheckCode : 0x"  << std::hex << dumpHeader->BugCheckCode << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[0] : " << dumpHeader->BugCheckCodeParameters[0] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[1] : " << dumpHeader->BugCheckCodeParameters[1] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[2] : " << dumpHeader->BugCheckCodeParameters[2] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[3] : " << dumpHeader->BugCheckCodeParameters[3] << std::endl;
        std::cout << std::oct;
    }

    UnmapViewOfFile(mapAddress);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <MinidumpFile>" << std::endl;
        return 1;
    }

    AnalyzeMinidump(argv[1]);

    system("pause");
    return 0;
}
