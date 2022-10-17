#include <ntddk.h>

void DriverUnload(PDRIVER_OBJECT DriverObject);

void OnProcessCreateCloseNotify(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create);
NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DriverWrited(PDEVICE_OBJECT DeviceObject, PIRP Irp);


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);

	DriverObject->DriverUnload = DriverUnload;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ProcessInterceptor");
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\\\??\\ProcessInterceptor");
	PDEVICE_OBJECT DeviceObject;

	auto status = STATUS_SUCCESS;

	do
	{
		// 1. create device object
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, false, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create device. (%08x)", status));
			break;
		}

		// 2. create symbolicLink
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create symbolicLink. (%08x)", status));
			break;
		}

		// 3. register create process notify
		// If NotifyType is PsCreateThreadNotifyNonSystem, 
		// the PsSetCreateThreadNotifyRoutineEx routine differs from 
		// PsSetCreateThreadNotifyRoutine in the context in which the callback is executed. 
		// With PsSetCreateThreadNotifyRoutine, the callback is executed on the creator thread. 
		// With PsSetCreateThreadNotifyRoutineEx, the callback is executed on the newly created thread.
		status = PsSetCreateProcessNotifyRoutine(OnProcessCreateCloseNotify, false);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to register process notify. (%08x)", status));
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
			KdPrint(("Failed to delete symbolLink. (%08x)", status));
		}

		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	auto status = PsSetCreateProcessNotifyRoutine(OnProcessCreateCloseNotify, true);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to unregister process notify. (%08x)", status));
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\\\??\\ProcessInterceptor");
	status = IoDeleteSymbolicLink(&symLink);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to delete symbolLink. (%08x)", status));
	}
	if (DriverObject->DeviceObject) {
		IoDeleteDevice(DriverObject->DeviceObject);
	}
}

void OnProcessCreateCloseNotify(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create) {

}

NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto status = STATUS_SUCCESS;

	return status;
}

NTSTATUS DriverWrited(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto status = STATUS_SUCCESS;

	return status;
}



