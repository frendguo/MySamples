#include "Main.h"

#define DRIVER_TAG 'pint'

Globals g_Globals;

void DriverUnload(PDRIVER_OBJECT DriverObject);

extern "C" NTKERNELAPI
UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);

void OnProcessCreateCloseNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DriverWrited(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS CompleteIo(PIRP& Irp, NTSTATUS status, int count = 0) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

void PushItem(SINGLE_LIST_ENTRY& entry) {
	if (g_Globals.ItemCount > 1024) {
		// 太大了，移除一个
		auto singleEntry = PopEntryList(&g_Globals.ItemHeader);
		g_Globals.ItemCount--;
		auto info = CONTAINING_RECORD(singleEntry, FullItem<char*>, Entry);
		ExFreePool(info->Data);
		ExFreePool(info);
	}

	g_Globals.ItemCount++;
	PushEntryList(&g_Globals.ItemHeader, &entry);
}

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	DriverObject->DriverUnload = DriverUnload;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ProcessInterceptor");
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\ProcessInterceptor");
	PDEVICE_OBJECT DeviceObject;

	auto status = STATUS_SUCCESS;

	// 0. init g_Globals
	g_Globals.ItemHeader.Next = NULL;
	g_Globals.ItemCount = 0;

	do
	{
		// 1. create device object
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, false, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create device. (0X%08X)\n", status));
			break;
		}

		// 设置直接缓冲区
		DeviceObject->Flags |= DO_DIRECT_IO;

		// 2. create symbolicLink
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create symbolicLink. (0X%08X)\n", status));
			break;
		}

		// 3. register create process notify
		// 
		// If NotifyType is PsCreateThreadNotifyNonSystem, 
		// the PsSetCreateThreadNotifyRoutineEx routine differs from 
		// PsSetCreateThreadNotifyRoutine in the context in which the callback is executed. 
		// With PsSetCreateThreadNotifyRoutine, the callback is executed on the creator thread. 
		// With PsSetCreateThreadNotifyRoutineEx, the callback is executed on the newly created thread.
		// 
		// ref: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreatethreadnotifyroutineex
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessCreateCloseNotify, false);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to register process notify. (0X%08X)\n", status));
			break;
		}

		// 4. register major routine
		DriverObject->MajorFunction[IRP_MJ_CREATE] =
			DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateOrClose;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DriverWrited;
	} while (false);

	// handle exception session
	if (!NT_SUCCESS(status)) {
		status = IoDeleteSymbolicLink(&symLink);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to delete symbolLink. (0X%08X)\n", status));
		}

		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}

	return status;
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	auto status = PsSetCreateProcessNotifyRoutineEx(OnProcessCreateCloseNotify, true);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to unregister process notify. (0X%08X)\n", status));
	}

	while (g_Globals.ItemCount)
	{
		g_Globals.ItemCount--;
		auto entry = PopEntryList(&g_Globals.ItemHeader);
		auto info = CONTAINING_RECORD(entry, FullItem<char*>, Entry);

		ExFreePool(info->Data);
		ExFreePool(info);
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\\\??\\ProcessInterceptor");
	status = IoDeleteSymbolicLink(&symLink);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to delete symbolLink. (0X%08X)\n", status));
	}
	if (DriverObject->DeviceObject) {
		IoDeleteDevice(DriverObject->DeviceObject);
	}
}

void OnProcessCreateCloseNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(ProcessId);
	
	// 1. 如果是进程退出，直接返回
	if (!CreateInfo) {
		return;
	}

	UCHAR* procName = PsGetProcessImageFileName(Process);
	KdPrint(("[%s] is created.\n", procName));
	// 2. 如果待阻止的列表为空，直接返回
	if (g_Globals.ItemCount <= 0) {
		return;
	}

	// 3. 遍历待阻止的列表，如果该进程在待阻止列表内，阻止
	auto entry = g_Globals.ItemHeader.Next;
	while (entry != nullptr)
	{
		auto info = CONTAINING_RECORD(entry, FullItem<char*>, Entry);
		// 比较和进程名是否一致
		auto len = strlen((char*)procName);
		if (!strncmp((char*)procName, info->Data, len)) {
			CreateInfo->CreationStatus = STATUS_UNSUCCESSFUL;
			return;
		}

		entry = entry->Next;
	}
}

NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto status = STATUS_SUCCESS;

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);

	return status;
}

NTSTATUS DriverWrited(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto status = STATUS_SUCCESS;

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Write.Length;

	auto p = (CHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, MM_PAGE_PRIORITY::NormalPagePriority);
	if (p == nullptr) {
		KdPrint(("failed to get mdl memory. \n"));
		return CompleteIo(Irp, STATUS_PARITY_ERROR);
	}

	auto info = (FullItem<char*>*)ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FullItem<char*>), DRIVER_TAG);
	auto data = (char*)ExAllocatePool2(POOL_FLAG_PAGED, len, DRIVER_TAG);

	if (info == nullptr || data == nullptr) {
		KdPrint(("failed to allocate memory\n"));
		return CompleteIo(Irp, STATUS_RESOURCE_DATA_NOT_FOUND);
	}

	memcpy(data, p, len);
	info->Data = data;

	PushItem(info->Entry);

	return CompleteIo(Irp, status, len);
}



