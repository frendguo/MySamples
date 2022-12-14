#include "SysMon.h"
#include "SysMonCommon.h"
#include "AutoLock.h"

Globals g_Globals;

#define DRIVER_PREFIX "SYSMON: "
#define DRIVER_TAG 'sysm'

void PushItem(LIST_ENTRY* entry);
void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create);
void OnLoadImageNotify(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo);

void SysMonUnload(_DRIVER_OBJECT* DriverObject);

DRIVER_DISPATCH SysMonCreateClose, SysMonRead;

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	auto status = STATUS_SUCCESS;

	// init g_Globals
	InitializeListHead(&g_Globals.ItemsHeader);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	// 符号链接名称应该是：\??\SysMon
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\SysMon");
	// 设备名称应该是 \Device\SysMon
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\SysMon");

	bool symLinkCreated = false;

	do
	{
		// 1. 创建 DeviceObject
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, true, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
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
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, false);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register process create notify.(0x%08X)\n", status));
			break;
		}

		// 4. 注册线程通知
		status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register thread create notify.(0x%08X)\n", status));
			break;
		}

		// 5. 注册映像载入通知
		status = PsSetLoadImageNotifyRoutine(OnLoadImageNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register load image notify.(0x%08X)\n", status));
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

void OnProcessNotify(PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(Process);

	if (CreateInfo) {
		// 进程创建
		auto allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = CreateInfo->CommandLine->Length;
		if (commandLineSize > 0) {
			allocSize += commandLineSize;
		}

		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, allocSize, DRIVER_TAG);
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
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
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

void OnThreadNotify(HANDLE ProcessId,
	HANDLE ThreadId,
	BOOLEAN Create) {

	auto info = (FullItem<ThreadCreateExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED,
		sizeof(FullItem<ThreadCreateExitInfo>), DRIVER_TAG);

	if (info == nullptr) {
		KdPrint((DRIVER_PREFIX "Failed to allocate pool to fill TheadCreateExitInfo!"));
		return;
	}

	auto& item = info->Data;
	KeQuerySystemTime(&item.Time);
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToULong(ThreadId);
	item.Size = sizeof(ThreadCreateExitInfo);
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;

	PushItem(&info->Entry);
}

void OnLoadImageNotify(
	PUNICODE_STRING FullImageName,
	HANDLE ProcessId,                // pid into which image is being mapped
	PIMAGE_INFO ImageInfo
) {
	auto info = (FullItem<LoadImageInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FullItem<LoadImageInfo>), DRIVER_TAG);
	if (info == nullptr) {
		KdPrint((DRIVER_PREFIX "Failed to allocate pool to fill LoadImageInfo!"));
		return;
	}

	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	item.ProcessId = HandleToLong(ProcessId);
	item.Type = ItemType::LoadImage;
	item.Size = sizeof(LoadImageInfo);
	item.ImageSize = ImageInfo->ImageSize;
	item.LoadAddress = ImageInfo->ImageBase;
	if (FullImageName) {
		memcpy(item.ImageFileName, FullImageName->Buffer, min(FullImageName->Length, MaxImageFileSize * sizeof(WCHAR)));
	}
	else {
		wcscpy_s(item.ImageFileName, L"(unknow)");
	}

	PushItem(&info->Entry);
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
	PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)OnProcessNotify, true);
	PsRemoveCreateThreadNotifyRoutine((PCREATE_THREAD_NOTIFY_ROUTINE)OnThreadNotify);
	PsRemoveLoadImageNotifyRoutine(OnLoadImageNotify);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	while (!IsListEmpty(&g_Globals.ItemsHeader))
	{
		auto entry = RemoveHeadList(&g_Globals.ItemsHeader);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
}

NTSTATUS SysMonRead(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto status = STATUS_SUCCESS;

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	auto count = 0;

	NT_ASSERT(Irp->MdlAddress);

	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else {
		AutoLock<FastMutex> lock(g_Globals.Mutex);

		while (true)
		{
			if (IsListEmpty(&g_Globals.ItemsHeader)) {
				break;
			}

			auto entry = RemoveHeadList(&g_Globals.ItemsHeader);
			// 通过成员来找结构的地址
			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto size = info->Data.Size;

			if (len < size) {
				// 缓冲区已经满了，把移除出来的 entry 
				InsertHeadList(&g_Globals.ItemsHeader, entry);
				break;
			}

			memcpy(buffer, &info->Data, size);
			g_Globals.ItemCount--;
			len -= size;
			buffer += size;
			count += size;

			ExFreePool(info);
		}
	}
	// 收尾
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, 0);

	return status;
}

NTSTATUS SysMonCreateClose(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);

	return STATUS_SUCCESS;
}
