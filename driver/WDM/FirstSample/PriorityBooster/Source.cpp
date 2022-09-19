#include "PriorityBoosterCommon.h"
#include <ntifs.h>
#include <ntddk.h>

UNICODE_STRING g_symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);

NTSTATUS
PriorityBoosterCreateClose(
	_In_ _DEVICE_OBJECT* DeviceObject,
	_Inout_ _IRP* Irp
);

NTSTATUS
PriorityBoosterDeviceControl(
	_In_ _DEVICE_OBJECT* DeviceObject,
	_Inout_ _IRP* Irp
);

extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	DriverObject->DriverUnload = PriorityBoosterUnload;

	// 设置支持的分发例程
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	// 创建设备对象
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create object (0x%08X)\n", status));
		return status;
	}

	// 创建符号链接
	status = IoCreateSymbolicLink(&g_symLink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	IoDeleteSymbolicLink(&g_symLink);

	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
PriorityBoosterCreateClose(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
PriorityBoosterDeviceControl(
	_In_ _DEVICE_OBJECT* DeviceObject,
	_Inout_ _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_FLT_BUFFER_TOO_SMALL;
			break;
		}

		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
		if (!NT_SUCCESS(status)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		KeSetPriorityThread((PKTHREAD)thread, data->Priority);
		ObDereferenceObject(thread);

		KdPrint(("Thread priority change for d% to d% succeeded!\n", data->ThreadId, data->Priority));
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}