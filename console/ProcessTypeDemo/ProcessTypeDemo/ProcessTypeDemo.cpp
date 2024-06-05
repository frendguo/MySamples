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
#include <map>

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

#define MIN_WINDOW_WIDTH    200
#define MIN_WINDOW_HEIGHT   200
bool IsBackgroudProcess(DWORD pid) {
    DWORD sessionId = -1;

    // subsystem 为 cui
    if (GetProcessSubsystem(pid) == IMAGE_SUBSYSTEM_WINDOWS_CUI) {
        std::cout << "subsystem is cui." << std::endl;
        return true;
    }

    // session 为 0
    if (ProcessIdToSessionId(pid, &sessionId) && sessionId == 0) {
        std::cout << "session id is zero." << std::endl;
        return true;
    }

    // UWP 进程 isbackgroud
    // TODO: UWP 本身有后台任务的限制 先忽略

    // 窗口信息
    std::vector<HWND> windows = GetProcessWindows(pid);
    if (windows.size() < 1) {
        std::cout << "no windows." << std::endl;
        return true;
    }

    // 如果所有窗口都不可见，或者大小都小于最小值，是后台进程
    for (auto hwnd : windows) {
        // 如果不可见，继续下一个窗口
        if (!IsWindowVisible(hwnd)) {
            continue;
        }
        // 如果可见，判断窗口的大小
        RECT rect;
        if (!GetWindowRect(hwnd, &rect)) {
            continue;
        }

        // 如果窗口大小大于最小值，不是后台进程
        auto width = std::abs(rect.right - rect.left);
        auto height = std::abs(rect.top - rect.bottom);
        if (width > MIN_WINDOW_WIDTH || height > MIN_WINDOW_HEIGHT) {
            std::cout << "window is visible. window size is: " << width << "*" << height << std::endl;
            return false;
        }
    }
    
    return true;
}

bool IsProcessSuspended(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return false;
    }

    PROCESS_EXTENDED_BASIC_INFORMATION pebi;

    NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pebi, sizeof(pebi), NULL);
    if (NT_SUCCESS(status) && pebi.IsFrozen)
    {
        CloseHandle(hProcess);
        return true;
    }

    // 遍历线程
    HANDLE hThread = nullptr;

    while (NtGetNextThread(hProcess, hThread, THREAD_QUERY_LIMITED_INFORMATION | THREAD_TERMINATE, 0, 0, &hThread) == STATUS_SUCCESS) {
        ULONG isTerminated = 0;
        ULONG retLen;
        status = NtQueryInformationThread(hThread, ThreadIsTerminated, &isTerminated, sizeof(isTerminated), &retLen);
        if (NT_SUCCESS(status) && isTerminated)
        {
            continue;
        }

        int suspendCount = 0;
        status = NtQueryInformationThread(hThread, ThreadSuspendCount, &suspendCount, sizeof(suspendCount), NULL);

        if (!NT_SUCCESS(status) || suspendCount < 1) {
            CloseHandle(hThread);
            CloseHandle(hProcess);
            return false;
        }
    }

    if (hThread == nullptr) {
        return false;
    }

    return true;
}

DWORD GetActiveProcessId() {
    auto hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
        return -1;
    }

    DWORD processID;
    GetWindowThreadProcessId(hwnd, &processID);

    return processID;
}

enum ProcessType
{
    None,
    SuspendProcess,
    BackgroundProcess,
    ForegroundProcess,
    ActiveForegroundProcess,
    InteractionProcess
};

ProcessType GetProcessType(DWORD pid) {
    ProcessType type = ProcessType::None;
    bool res = false;
    // 判断进程是否被挂起
    if (IsProcessSuspended(pid)) {
        return ProcessType::SuspendProcess;
    }

    // 判断进程是否是后台进程
    if (IsBackgroudProcess(pid)) {
        return ProcessType::BackgroundProcess;
    }

    // 判断进程是否是交互进程（全局唯一）
    DWORD activePid = GetActiveProcessId();
    if (activePid == pid) {
        return ProcessType::InteractionProcess;
    }

    // 判断进程是否是前台活跃进程
    // TODO：这里得依赖 CPU time，就得有实时监控

    return ProcessType::ForegroundProcess;
}

void ProcessTypeTestByPid() {
    std::cout << "Please enter the PID of the process you want to check: " << std::endl;
    DWORD pid;
    std::cin >> pid;

    auto start = std::chrono::high_resolution_clock::now();
    ProcessType type = GetProcessType(pid);

    auto end = std::chrono::high_resolution_clock::now();

    switch (type)
    {
    case ProcessType::SuspendProcess:
        std::cout << "The process is suspended." << std::endl;
        break;
    case ProcessType::BackgroundProcess:
        std::cout << "The process is background process." << std::endl;
        break;
    case ProcessType::ForegroundProcess:
        std::cout << "The process is foreground process." << std::endl;
        break;
    case ProcessType::ActiveForegroundProcess:
        std::cout << "The process is active foreground process." << std::endl;
        break;
    default:
        std::cout << "The process is unknow." << std::endl;
        break;
    }

    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
}

void AllProcessTypeInSystem() {
    std::vector<DWORD> pids;
    auto start = std::chrono::high_resolution_clock::now();
    EnumProcess(pids);

    std::map<int, ProcessType> processTypes;

    for (auto pid : pids) {
        ProcessType type = GetProcessType(pid);

        processTypes.insert(std::make_pair(pid, type));
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "-------------------------------------------" << std::endl;

    for (auto& [pid, type] : processTypes) {
        std::wstring processName = GetProcessName(pid);
        std::wcout << "PID: " << pid << " Name: " << processName << " Type: ";
        
        switch (type)
        {
        case ProcessType::SuspendProcess:
            std::cout << "SuspendProcess";
            break;
        case ProcessType::BackgroundProcess:
            std::cout << "BackgroundProcess";
            break;
        case ProcessType::ForegroundProcess:
            std::cout << "ForegroundProcess";
            break;
        case ProcessType::ActiveForegroundProcess:
            std::cout << "ActiveForegroundProcess";
            break;
        default:
            std::cout << "Unknown";
            break;
        }
        std::cout << std::endl;
    }

    std::cout << "cout: " << pids.size() << std::endl;
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
}

int main()
{
    while (true)
    {
        system("cls");

        std::cout << "Please chose the mode you want to use: " << std::endl;
        std::cout << "1. Get process type by pid." << std::endl;
        std::cout << "2. Get all process type in system." << std::endl;
        int mode;
        std::cin >> mode;

        switch (mode)
        {
        case 1:
            ProcessTypeTestByPid();
            break;
        case 2:
            AllProcessTypeInSystem();
            break;
        }

        system("pause");
    }
    return 0;
}