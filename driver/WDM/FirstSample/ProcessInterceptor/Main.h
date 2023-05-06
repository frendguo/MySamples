#pragma once
#include <ntifs.h>
#include <ntddk.h>

template<typename T>
struct FullItem
{
	SINGLE_LIST_ENTRY Entry;
	T Data;
};

struct Globals
{
	SINGLE_LIST_ENTRY ItemHeader;
	int ItemCount;
};
