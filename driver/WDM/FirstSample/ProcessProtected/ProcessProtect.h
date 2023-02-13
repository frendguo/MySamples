#pragma once

#include <ntdef.h>
#include "FastMutex.h"

#define PROCESS_PROTECT_NAME L"ProcessProtect"
#define DRIVER_PREFIX "ProcessProtect: "
#define PROCESS_TERMINATE 1

const int maxProcessCount = 256;

struct Globals {
  int processCount;
  PUNICODE_STRING process[maxProcessCount];
  FastMutex Lock;
  PVOID RegHandle;

  void Init() { Lock.Init(); }
};
