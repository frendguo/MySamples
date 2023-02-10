#include <ntddk.h>
#include <fltKernel.h>


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT Driver, PUNICODE_STRING RegisterPath) {
	NTSTATUS status = STATUS_SUCCESS;
	
	UNREFERENCED_PARAMETER(RegisterPath);

	FltRegisterFilter(Driver, )


	return status;
}



