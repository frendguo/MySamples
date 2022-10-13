// SysMonClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <Windows.h>
#include "../SysMon/SysMonCommon.h"
#include <string>

int Error(const char* msg) {
	printf("%s, error code is %d\n", msg, GetLastError());
	return -1;
}

void DisplayTime(const LARGE_INTEGER& time) {
	SYSTEMTIME st{};
	FileTimeToSystemTime((FILETIME*)&time, &st);

	printf("%02d:%02d:%02d.%03d ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void DisplayInfo(BYTE* buffer, DWORD size) {
	auto count = size;
	while (count > 0) {
		auto header = (ItemHeader*)buffer;
		std::wstring commandLine;

		switch (header->Type)
		{
		case ItemType::ProcessExit:
		{
			DisplayTime(header->Time);
			auto info = (ProcessExitInfo*)header;
			printf("Process %d Exited.\n", info->ProcessId);
			break;
		}
		case ItemType::ProcessCreate:
		{
			DisplayTime(header->Time);
			auto info = (ProcessCreateInfo*)header;
			std::wstring commandLine((WCHAR*)(buffer + info->CommandLineOffset), info->CommandLineLength);
			// 这里要用 %ws
			printf("Process %d Created. Commandline is %ws\n", info->ProcessId, commandLine.c_str());
			break;
		}
		case ItemType::ThreadCreate:
		{
			DisplayTime(header->Time);
			auto info = (ThreadCreateExitInfo*)header;
			printf("Thread %d created in process %d. \n", info->ThreadId, info->ProcessId);
			break;
		}
		case ItemType::ThreadExit:
		{
			DisplayTime(header->Time);
			auto info = (ThreadCreateExitInfo*)header;
			printf("Thread %d deleted in process %d. \n", info->ThreadId, info->ProcessId);
		}
		case ItemType::LoadImage:
		{
			DisplayTime(header->Time);
			auto info = (LoadImageInfo*)header;
			printf("Image is loaded in process %d, Image name is: %ws, size is %llu \n", 
				info->ProcessId, info->ImageFileName, info->ImageSize);
			break;
		}
		default:
			break;
		}
		buffer += header->Size;
		count -= header->Size;
	}
}

int main()
{
	auto hFile = CreateFile(L"\\\\.\\SysMon", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return Error("Filed to open file.");
	}

	BYTE buffer[1 << 16]{};
	while (true)
	{
		DWORD bytes;
		if (!ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr)) {
			return Error("Filed to read.");
		}

		if (bytes != 0) {
			DisplayInfo(buffer, bytes);
		}

		Sleep(200);
	}

	return 0;
}

