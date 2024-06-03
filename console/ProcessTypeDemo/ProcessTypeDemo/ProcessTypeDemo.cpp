// ProcessTypeDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <phnt_windows.h>
#include <phnt.h>
#include <tlhelp32.h>
#include <conio.h>
#include <psapi.h>
#include <vector>
#include <chrono>

bool GetProcessExtendedBasicInformation(DWORD pid, PPROCESS_EXTENDED_BASIC_INFORMATION ppebi) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

    if (hProcess == NULL) {
        std::cout << "OpenProcess failed with error " << GetLastError() << std::endl;
        return false;
    }

    NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, ppebi, sizeof(PROCESS_EXTENDED_BASIC_INFORMATION), NULL);
    CloseHandle(hProcess);

    if (!NT_SUCCESS(status)) {
        std::cout << "NtQueryInformationProcess failed with status " << status << std::endl;
        return false;
    }

    return true;
}

bool IsBackgroudProcess(DWORD pid) {
    PROCESS_EXTENDED_BASIC_INFORMATION pebi = { 0 };

    if (!GetProcessExtendedBasicInformation(pid, &pebi)) {
        return false;
    }

    return pebi.IsBackground;
}

std::wstring GetProcessExePath(DWORD processID) {
    WCHAR exePath[MAX_PATH] = L"<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH) == 0) {
            wcscpy_s(exePath, L"<error>");
        }
        CloseHandle(hProcess);
    }
    return std::wstring(exePath);
}

int GetImageSubsystem(const std::wstring& exePath) {
    HANDLE hFile = CreateFile(exePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed: " << GetLastError() << std::endl;
        return -1;
    }

    DWORD dwRead;
    IMAGE_DOS_HEADER dosHeader;
    if (!ReadFile(hFile, &dosHeader, sizeof(IMAGE_DOS_HEADER), &dwRead, NULL)) {
        std::cerr << "ReadFile failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Invalid DOS header" << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN);

    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadFile(hFile, &ntHeaders, sizeof(IMAGE_NT_HEADERS), &dwRead, NULL)) {
        std::cerr << "ReadFile failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Invalid NT header" << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    CloseHandle(hFile);
    return ntHeaders.OptionalHeader.Subsystem;
}

int GetProcessSubsystem(DWORD pid) {
    std::wstring exePath = GetProcessExePath(pid);
    if (exePath == L"<unknown>" || exePath == L"<error>") {
        return -1;
    }

    return GetImageSubsystem(exePath);

}

std::wstring GetProcessName(HANDLE hProcess) {
    WCHAR processName[MAX_PATH] = L"<unknown>";
    HMODULE hMod;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        GetModuleBaseName(hProcess, hMod, processName, sizeof(processName) / sizeof(WCHAR));
    }
    return std::wstring(processName);

}

std::wstring GetProcessName(DWORD processID) {
    std::wstring processNameStr = L"<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        processNameStr = GetProcessName(hProcess);
        CloseHandle(hProcess);
    }
    return processNameStr;
}

std::wstring ConvertToSubsystemName(int subsystem) {
    switch (subsystem) {
    case IMAGE_SUBSYSTEM_UNKNOWN:
        return L"UNKNOWN";
    case IMAGE_SUBSYSTEM_NATIVE:
        return L"NATIVE";
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        return L"WINDOWS_GUI";
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        return L"WINDOWS_CUI";
    case IMAGE_SUBSYSTEM_OS2_CUI:
        return L"OS2_CUI";
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        return L"POSIX_CUI";
    case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
        return L"WINDOWS_CE_GUI";
    case IMAGE_SUBSYSTEM_EFI_APPLICATION:
        return L"EFI_APPLICATION";
    case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
        return L"EFI_BOOT_SERVICE_DRIVER";
    case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
        return L"EFI_RUNTIME_DRIVER";
    case IMAGE_SUBSYSTEM_EFI_ROM:
        return L"EFI_ROM";
    case IMAGE_SUBSYSTEM_XBOX:
        return L"XBOX";
    case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
        return L"WINDOWS_BOOT_APPLICATION";
    default:
        return L"UNKNOWN";
    }
}

struct EnumWindowsData {
    DWORD processID;
    std::vector<HWND> windows;
};

// 获取指定进程 ID 的所有窗口句柄
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processID;
    GetWindowThreadProcessId(hwnd, &processID);

    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);

    if (processID == data->processID) {
        data->windows.push_back(hwnd);
    }

    return TRUE;
}

std::vector<HWND> GetProcessWindows(DWORD processID) {
    EnumWindowsData data;
    data.processID = processID;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.windows;
}

// 获取窗口标题
std::wstring GetWindowTitle(HWND hwnd) {
    int length = GetWindowTextLength(hwnd) + 1;
    std::vector<wchar_t> buffer(length);
    GetWindowText(hwnd, buffer.data(), length);
    return std::wstring(buffer.data());
}

// 获取窗口状态
std::wstring GetWindowState(HWND hwnd) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &wp)) {
        switch (wp.showCmd) {
        case SW_SHOWMINIMIZED:
            return L"Minimized";
        case SW_SHOWMAXIMIZED:
            return L"Maximized";
        case SW_SHOWNORMAL:
        default:
            return L"Normal";
        }
    }
    return L"Unknown";
}

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

int main()
{
    /*std::cout << "Please enter the PID of the process you want to check: " << std::endl;
    DWORD pid;
    std::cin >> pid;*/
    std::vector<DWORD> pids;
    std::vector<DWORD> pids2;

    auto start = std::chrono::high_resolution_clock::now();

    EnumProcess(pids);

    auto end = std::chrono::high_resolution_clock::now();

    EnumProcessBySnap(pids2);

    auto end2 = std::chrono::high_resolution_clock::now();

    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
    std::cout << "Time2: " << std::chrono::duration_cast<std::chrono::microseconds>(end2 - end).count() << "us" << std::endl;

    //std::vector<std::wstring> hasWindowprocess;
    //

    //HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    //if (hProcessSnap == INVALID_HANDLE_VALUE) {
    //    std::cerr << "CreateToolhelp32Snapshot (of processes) failed: " << GetLastError() << std::endl;
    //    return 1;
    //}

    //PROCESSENTRY32 pe32;
    //pe32.dwSize = sizeof(PROCESSENTRY32);

    //// Retrieve information about the first process
    //if (!Process32First(hProcessSnap, &pe32)) {
    //    std::cerr << "Process32First failed: " << GetLastError() << std::endl; // show cause of failure
    //    CloseHandle(hProcessSnap);          // clean the snapshot object
    //    return 1;
    //}

    //do
    //{
    //    // std::cout << "ProcessID: " << pe32.th32ProcessID << "IsBackground: " << IsBackgroudProcess(pe32.th32ProcessID) << std::endl;
    //    std::wstring processName = GetProcessName(pe32.th32ProcessID);
    //    if (processName == L"<unknown>") {
    //        continue;
    //    }
    //    std::wcout << "Processname: " << processName << ";Subsystem: " << ConvertToSubsystemName(GetProcessSubsystem(pe32.th32ProcessID)) << std::endl;

    //    auto windows = GetProcessWindows(pe32.th32ProcessID);
    //    if (windows.size() < 1) {
    //        continue;
    //    }

    //    hasWindowprocess.push_back(processName);
    //    std::wcout << "Windows: " << std::endl;
    //    for (auto hwnd : windows) {
    //        std::wcout << "  Title: " << GetWindowTitle(hwnd) << ";State: " << GetWindowState(hwnd) << std::endl;
    //    }

    //} while (Process32Next(hProcessSnap, &pe32));

    //std::wcout << "Processes with windows: " << std::endl;
    //for (std::wstring p : hasWindowprocess) {
    //    std::wcout << "1111-" << p << std::endl;
    //}

    system("pause");
    return 0;
}