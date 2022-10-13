#pragma once

enum class ItemType : short
{
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	LoadImage
};

struct ItemHeader
{
	ItemType Type;
	USHORT Size;
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader
{
	ULONG ProcessId;
};

struct ProcessCreateInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ParentProcessId;
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
};

struct ThreadCreateExitInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ThreadId;
};

const int MaxImageFileSize = 300;
struct LoadImageInfo : ItemHeader
{
	ULONG ProcessId;
	void* LoadAddress;
	ULONG_PTR ImageSize;
	WCHAR ImageFileName[MaxImageFileSize + 1];
};
