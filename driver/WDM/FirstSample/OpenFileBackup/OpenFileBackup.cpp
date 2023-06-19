#include <fltKernel.h>
#include <ntddk.h>

PFLT_FILTER g_filter;
VOID DriverUnload(_DRIVER_OBJECT* DriverObject);
NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS WriteFilePreCallback(PFLT_CALLBACK_DATA Data,
                                               PCFLT_RELATED_OBJECTS FltObjects,
                                               PVOID* CompletionContext);
NTSTATUS FileBackupInstanceSetupCallback(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_SETUP_FLAGS Flags,
    DEVICE_TYPE VolumeDeviceType,
    FLT_FILESYSTEM_TYPE VolumeFilesystemType);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegisterPath) {
  RegisterPath;

  UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\FileBackUp");

  PDEVICE_OBJECT deviceObject;
  FLT_OPERATION_REGISTRATION callbacks[] = {
      {IRP_MJ_WRITE, 0, WriteFilePreCallback, NULL}, {IRP_MJ_OPERATION_END}};

  NTSTATUS status = STATUS_SUCCESS;
  FLT_REGISTRATION reg = {
      sizeof(FLT_REGISTRATION),  // Size
      FLT_REGISTRATION_VERSION,  // Version
      FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS |
          FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME,  // Flags
      NULL,                                       // ContextRegistration
      callbacks,                                  // OperationRegistration
      FilterUnload,                               // FilterUnloadCallback
      FileBackupInstanceSetupCallback,            // InstanceSetupCallback
      NULL,  // InstanceQueryTeardownCallback
      NULL,  // InstanceTeardownStartCallback
      NULL,  // InstanceTeardownCompleteCallback
      NULL,  // GenerateFileNameCallback
      NULL,  // NormalizeNameComponentCallback
      NULL,  // TransactionNotificationCallback
      NULL,  // NormalizeNameComponentExCallback
      NULL,  // SectionNotificationCallback
  };

  do {
    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN,
                            0, false, &deviceObject);
    if (!NT_SUCCESS(status)) {
      KdPrint(("Fail to create device. status code is (0x%08X)\n", status));
      break;
    }

    DriverObject->DriverUnload = DriverUnload;
    status = FltRegisterFilter(DriverObject, &reg, &g_filter);
    if (!NT_SUCCESS(status)) {
      KdPrint(("Fail to regist filter. status code is (0x%08X)\n", status));
      break;
    }

    status = FltStartFiltering(g_filter);
    if (!NT_SUCCESS(status)) {
      KdPrint(("Fail to start filter. status code is (0x%08X)\n", status));
      break;
    }
  } while (false);

  if (!NT_SUCCESS(status)) {
    if (g_filter) {
      FltUnregisterFilter(g_filter);
    }

    if (deviceObject) {
      IoDeleteDevice(deviceObject);
    }
  }

  return status;
}

NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
  UNREFERENCED_PARAMETER(Flags);

  return STATUS_SUCCESS;
}

VOID DriverUnload(_DRIVER_OBJECT* DriverObject) {
  if (g_filter) {
    FltUnregisterFilter(g_filter);
  }

  if (DriverObject->DeviceObject) {
    IoDeleteDevice(DriverObject->DeviceObject);
  }
}

NTSTATUS FileBackupInstanceSetupCallback(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_SETUP_FLAGS Flags,
    DEVICE_TYPE VolumeDeviceType,
    FLT_FILESYSTEM_TYPE VolumeFilesystemType) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(Flags);
  UNREFERENCED_PARAMETER(VolumeDeviceType);

  if (VolumeFilesystemType != FLT_FSTYPE_NTFS) {
    KdPrint(("Not attach to non-NTFS volume.\n"));
    return STATUS_FLT_DO_NOT_ATTACH;
  }

  return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS WriteFilePreCallback(PFLT_CALLBACK_DATA Data,
                                               PCFLT_RELATED_OBJECTS FltObjects,
                                               PVOID* CompletionContext) {
  UNREFERENCED_PARAMETER(CompletionContext);
  UNREFERENCED_PARAMETER(FltObjects);

  PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
  if (iopb->MajorFunction != IRP_MJ_WRITE) {
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }

  DbgBreakPoint();

  return FLT_PREOP_SUCCESS_NO_CALLBACK;
}