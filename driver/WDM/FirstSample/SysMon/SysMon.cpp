#include "SysMon.h"
#include "SysMonCommon.h"
#include "AutoLock.h"

Globals g_Globals;

#define DRIVER_PREFIX "SYSMON: "
#define DRIVER_TAG 'sysm'

void PushItem(LIST_ENTRY* entry);
void OnProcessNotify(HANDLE Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo);

void SysMonUnload(_DRIVER_OBJECT* DriverObject);

PDRIVER_DISPATCH SysMonCreateClose, SysMonRead;

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	auto status = STATUS_SUCCESS;

	// init g_Globals
	InitializeListHead(&g_Globals.ItemsHeader);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\?\\SysMon");
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\SysMon");

	bool symLinkCreated = false;

	do
	{
		// 1. 创建 DeviceObject
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, true, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n"), status);
			break;
		}
		// 访问用户缓冲区采用直接IO的方式
		DeviceObject->Flags |= DO_DIRECT_IO;

		// 2. 创建 symbolLink
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbol link.(0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;

		// 3. 注册进程通知
		status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)OnProcessNotify, false);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register process create notify.(0x%08X)\n", status));
			break;
		}

	} while (false);

	// 处理失败的场景
	if (!NT_SUCCESS(status)) {
		if (symLinkCreated) {
			IoDeleteSymbolicLink(&symLink);
		}
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}

	DriverObject->DriverUnload = SysMonUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = SysMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;

	return status;
}

void OnProcessNotify(HANDLE Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo) {
	if (CreateInfo) {
		// 进程创建
		auto allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = CreateInfo->CommandLine->Length;
		if (commandLineSize > 0) {
			allocSize += commandLineSize;
		}

		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation.\n"));
			return;
		}
		auto& item = info->Data;

		KeQuerySystemTimePrecise(&item.Time);
		item.ProcessId = HandleToLong(ProcessId);
		item.ParentProcessId = HandleToLong(CreateInfo->ParentProcessId);
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize;
		item.Type = ItemType::ProcessCreate;
		if (commandLineSize > 0) {
			memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}
		else {
			item.CommandLineLength = 0;
		}

		PushItem(&info->Entry);
	}
	else {
		// 进程退出
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation.\n"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToLong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);

		PushItem(&info->Entry);
	}
}

void PushItem(LIST_ENTRY* entry) {
	AutoLock<FastMutex> lock(g_Globals.Mutex);

	if (g_Globals.ItemCount > 1024) {
		auto head = RemoveHeadList(&g_Globals.ItemsHeader);
		g_Globals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}

	InsertTailList(&g_Globals.ItemsHeader, entry);
	g_Globals.ItemCount++;
}

void SysMonUnload(_DRIVER_OBJECT* DriverObject) {
	auto status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)OnProcessNotify, true);
}
