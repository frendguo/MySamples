#pragma once

#include <vector>
#include <phnt_windows.h>
#include <string>

struct ThreadInfo
{
    DWORD ThreadId;
    std::wstring ThreadName;
    int SuspendCount;
};

struct ProcessInfo
{
    DWORD ProcessId;
    std::wstring ProcessName;
    bool IsFreeze;
    std::vector<ThreadInfo> Threads;
};