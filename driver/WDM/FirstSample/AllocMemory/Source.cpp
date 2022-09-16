#include <ntddk.h>

#define DRIVERTAG 'hhhh'

UNICODE_STRING g_RegisterPath;

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	ExFreePool(g_RegisterPath.Buffer);
	KdPrint(("AllocMemory driver unload called!\n"));
}

// 把 RegisterPath 存储下来
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	DriverObject->DriverUnload = DriverUnload;

	// 分配空间
	// ExAllocatePoolWithTag 过期了，需要用 ExAllocatePool2 代替
	g_RegisterPath.Buffer = (WCHAR*)ExAllocatePool2(POOL_FLAG_PAGED, RegisterPath->Length, DRIVERTAG);
	if (g_RegisterPath.Buffer == nullptr) {
		KdPrint(("Failed to allocate memory\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	g_RegisterPath.MaximumLength = RegisterPath->Length;
	// todo: 怎么确保copy一定是成功的？
	RtlCopyUnicodeString(&g_RegisterPath, (PCUNICODE_STRING)RegisterPath);

	KdPrint(("Copied register path: %wZ\n", &g_RegisterPath));
	return STATUS_SUCCESS;
}