#include "PriorityBoosterCommon.h"

UNICODE_STRING g_symLink = RTL_CONSTANT_STRING(L"\\?\\PriorityBooster");

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
	DriverObject->DriverUnload = PriorityBoosterUnload;

	// 设置支持的分发例程
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	// 创建设备对象
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\PriorityBooster");
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

}

NTSTATUS
PriorityBoosterDeviceControl(
	_In_ _DEVICE_OBJECT* DeviceObject,
	_Inout_ _IRP* Irp
) {

}