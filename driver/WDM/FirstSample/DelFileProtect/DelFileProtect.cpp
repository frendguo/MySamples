#include <fltKernel.h>
#include <ntddk.h>

void DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS
FilterInstanceSetupCallback(PCFLT_RELATED_OBJECTS FltObjects,
                            FLT_INSTANCE_SETUP_FLAGS Flags,
                            DEVICE_TYPE VolumeDeviceType,
                            FLT_FILESYSTEM_TYPE VolumeFilesystemType);

FLT_PREOP_CALLBACK_STATUS FileCreatePreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

FLT_PREOP_CALLBACK_STATUS FileSetInfomationPreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

bool IsPreventProcess(PEPROCESS process);

PFLT_FILTER g_filter;

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegisterPath) {
  UNREFERENCED_PARAMETER(RegisterPath);
  NTSTATUS retStatus = STATUS_SUCCESS;

  UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DelFileProtect");
  UNICODE_STRING symlinkName = RTL_CONSTANT_STRING(L"\\??\\DelFileProtect");
  PDEVICE_OBJECT deviceObject;

  FLT_OPERATION_REGISTRATION callbacks[] = {
      {IRP_MJ_CREATE, 0, FileCreatePreCallback, NULL},
      {IRP_MJ_SET_INFORMATION, 0, FileSetInfomationPreCallback, NULL},
      {IRP_MJ_OPERATION_END}};

  FLT_REGISTRATION reg = {
      sizeof(FLT_REGISTRATION),  // Size
      FLT_REGISTRATION_VERSION,  // Version
      FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS |
          FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME,  // Flags
      NULL,                                       // ContextRegistration
      callbacks,                                  // OperationRegistration
      FilterUnload,                               // FilterUnloadCallback
      FilterInstanceSetupCallback,                // InstanceSetupCallback
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
    retStatus = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN,
                               0, false, &deviceObject);
    if (!NT_SUCCESS(retStatus)) {
      KdPrint(("Error: Fail to create device. (state=0X%X)", retStatus));
      break;
    }

    retStatus = IoCreateSymbolicLink(&symlinkName, &devName);
    if (!NT_SUCCESS(retStatus)) {
      KdPrint(("Error: Fail to create symbolic link. (state=0X%X)", retStatus));
      break;
    }

    retStatus = FltRegisterFilter(DriverObject, &reg, &g_filter);
    if (!NT_SUCCESS(retStatus)) {
      KdPrint(("Error: Fail to register filter. (state=0X%X)", retStatus));
      break;
    }

    retStatus = FltStartFiltering(g_filter);
    if (!NT_SUCCESS(retStatus)) {
      KdPrint(("Error: Fail to start filter. (state=0X%X)", retStatus));
      break;
    }
  } while (false);

  DriverObject->DriverUnload = DriverUnload;

  return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
  IoDeleteDevice(DriverObject->DeviceObject);

  UNICODE_STRING symlinkName = RTL_CONSTANT_STRING(L"\\??\\DelFileProtect");
  IoDeleteSymbolicLink(&symlinkName);
  FltUnregisterFilter(g_filter);
}

NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
  UNREFERENCED_PARAMETER(Flags);

  return STATUS_SUCCESS;
}

NTSTATUS
FilterInstanceSetupCallback(PCFLT_RELATED_OBJECTS FltObjects,
                            FLT_INSTANCE_SETUP_FLAGS Flags,
                            DEVICE_TYPE VolumeDeviceType,
                            FLT_FILESYSTEM_TYPE VolumeFilesystemType) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(Flags);
  UNREFERENCED_PARAMETER(VolumeDeviceType);
  UNREFERENCED_PARAMETER(VolumeFilesystemType);

  return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS FileCreatePreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(CompletionContext);

  if (Data->RequestorMode == KernelMode) {
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }

  const auto& params = Data->Iopb->Parameters.Create;
  if (params.Options & FILE_DELETE_ON_CLOSE) {
    // 删除操作
    PEPROCESS eProcess = PsGetThreadProcess(Data->Thread);
    if (IsPreventProcess(eProcess)) {
      Data->IoStatus.Status = STATUS_ACCESS_DENIED;
      return FLT_PREOP_COMPLETE;
    }
  }

  return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS FileSetInfomationPreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(CompletionContext);

  if (Data->RequestorMode == KernelMode) {
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }

  const auto& params = Data->Iopb->Parameters.SetFileInformation;
  if (params.FileInformationClass != FileDispositionInformation &&
      params.FileInformationClass != FileDispositionInformationEx) {
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }

  auto info = (FILE_DISPOSITION_INFORMATION*)params.InfoBuffer;
  if (info->DeleteFile) {
    PEPROCESS eProcess = PsGetThreadProcess(Data->Thread);
    if (IsPreventProcess(eProcess)) {
      KdPrint(("-----Prevent delete operation."));
      Data->IoStatus.Status = STATUS_ACCESS_DENIED;
      return FLT_PREOP_COMPLETE;
    }
  }

  return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


bool IsPreventProcess(PEPROCESS process) {
  PUNICODE_STRING imagePath;
  auto status = SeLocateProcessImageName(process, &imagePath);
  if (NT_SUCCESS(status)) {
    return wcsstr(imagePath->Buffer, L"\\System32\\cmd.exe") != NULL ||
           wcsstr(imagePath->Buffer, L"\\SysWOW64\\cmd.exe") != NULL;
  }
  return false;
}