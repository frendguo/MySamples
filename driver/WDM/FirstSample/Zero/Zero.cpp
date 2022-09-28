#include "pch.h"
#include "ZeroCommon.h"

#define DRIVER_PREFIX "ZERO: "

DRIVER_DISPATCH ZeroCreateClose, ZeroRead, ZeroWrite, ZeroDeviceControl;

VOID ZeroUnload(_DRIVER_OBJECT* DriverObject);

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

long long g_readTotal = 0;
long long g_writeTotal = 0;

// DriverEntry

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	// 初始化卸载方法
	DriverObject->DriverUnload = ZeroUnload;

	// 初始化分发实例（Dispatch Routine）
	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = ZeroCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = ZeroRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = ZeroWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ZeroDeviceControl;

	// 创建设备对象和符号链接
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Zero");
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Zero");

	auto status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = nullptr;

	do
	{
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}

		// 设置采用直接I/O读取缓冲区
		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbol link (0x%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}

	return status;
}

VOID ZeroUnload(_DRIVER_OBJECT* DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS
ZeroCreateClose(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	return CompleteIrp(Irp);
}

NTSTATUS
ZeroRead(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0) {
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);
	}

	auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);
	}

	memset(buffer, 0, len);
	g_readTotal += len;
	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}

NTSTATUS
ZeroWrite(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Write.Length;

	g_writeTotal += len;
	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}

NTSTATUS
ZeroDeviceControl(
	_DEVICE_OBJECT* DeviceObject,
	_IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto& ioControl = stack->Parameters.DeviceIoControl;
	if (ioControl.IoControlCode != IOCTL_ZERO_GET_STATE) {
		return CompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST);
	}

	if (ioControl.OutputBufferLength < sizeof(ZeroStates)) {
		return CompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL);
	}

	auto states = (ZeroStates*)Irp->AssociatedIrp.SystemBuffer;
	states->TotalRead = g_readTotal;
	states->TotalWrite = g_writeTotal;

	return CompleteIrp(Irp, STATUS_SUCCESS, sizeof(ZeroStates));
}

#pragma region Helper

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

#pragma endregion
