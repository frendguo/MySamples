#include <ntddk.h>

void DriverUnload(
	_In_ struct _DRIVER_OBJECT* DriverObject
) {
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint(("Driver unload called\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	//UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	RTL_OSVERSIONINFOW OsVersionInfo;
	PRTL_OSVERSIONINFOW POsVersionInfo = &OsVersionInfo;
	
	NTSTATUS status = RtlGetVersion(POsVersionInfo);
	if (NT_SUCCESS(status)) {
		KdPrint(("OS Version is %d.%d.%d\n",
			POsVersionInfo->dwMajorVersion, POsVersionInfo->dwMinorVersion, POsVersionInfo->dwBuildNumber));
	}
	else {
		KdPrint(("Get OsVersion fail. Status is %d\n", status));
	}

	KdPrint(("Driver initialized successfully\n"));

	return STATUS_SUCCESS;
}
