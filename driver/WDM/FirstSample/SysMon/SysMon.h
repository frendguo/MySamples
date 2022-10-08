#pragma once
#include <ntddk.h>
#include "FastMutex.h"

template<typename T>
struct FullItem
{
	LIST_ENTRY Entry;
	T Data;
};

struct Globals
{
	LIST_ENTRY ItemsHeader;
	int ItemCount;
	FastMutex Mutex;
};