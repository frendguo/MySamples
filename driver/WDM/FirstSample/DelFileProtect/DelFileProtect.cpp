#include <fltKernel.h>
#include <ntddk.h>
#include "DelFileProtect.h"

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

FLT_PREOP_CALLBACK_STATUS DirectoryControlPreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

FLT_POSTOP_CALLBACK_STATUS DirectoryControlPostCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags);

bool IsPreventProcess(PEPROCESS process);
bool IsHiddenDir(PUNICODE_STRING dir, PUNICODE_STRING file);

PFLT_FILTER g_filter;

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegisterPath) {
  UNREFERENCED_PARAMETER(RegisterPath);
  NTSTATUS retStatus = STATUS_SUCCESS;

  UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DelFileProtect");
  PDEVICE_OBJECT deviceObject;

  FLT_OPERATION_REGISTRATION callbacks[] = {
      {IRP_MJ_DIRECTORY_CONTROL, 0, DirectoryControlPreCallback,
       DirectoryControlPostCallback},
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

FLT_PREOP_CALLBACK_STATUS DirectoryControlPreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(CompletionContext);

  switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory
              .FileInformationClass) {
    case FileBothDirectoryInformation:
    case FileDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileIdBothDirectoryInformation:
    case FileIdFullDirectoryInformation:
    case FileNamesInformation:
      break;
    default:
      return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }

  return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS DirectoryControlPostCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags) {
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(CompletionContext);
  UNREFERENCED_PARAMETER(Flags);

  PFLT_FILE_NAME_INFORMATION fltName;

  if (!NT_SUCCESS(Data->IoStatus.Status)) {
    return FLT_POSTOP_FINISHED_PROCESSING;
  }

  auto status =
      FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &fltName);
  if (!NT_SUCCESS(status)) {
    KdPrint(("Fail to get fileNameInfomation.---\n"));
    return FLT_POSTOP_FINISHED_PROCESSING;
  }

  __try {
    switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory
                .FileInformationClass) {
      case FileBothDirectoryInformation:
        // powershell 访问进到这里
        KdPrint(("------FileBothDirectoryInformation------\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_BOTH_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileDirectoryInformation:
        KdPrint(("-----FileDirectoryInformation-----\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_DIRECTORY_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileFullDirectoryInformation:
        KdPrint(("-----FileFullDirectoryInformation-----\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_FULL_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileIdBothDirectoryInformation:
        // explorer 会进到这里
        KdPrint(("-----FileIdBothDirectoryInformation-----\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_ID_BOTH_DIR_INFORMATION)Data->Iopb->Parameters
                .DirectoryControl.QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileIdFullDirectoryInformation:
        KdPrint(("-----FileIdFullDirectoryInformation-----\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_ID_FULL_DIR_INFORMATION)Data->Iopb->Parameters
                .DirectoryControl.QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileNamesInformation:
        // 获取特定文件夹中的文件列表，fltname 中就是文件夹名称
        KdPrint(("-----FileNamesInformation-----\n"));
        CleanDirectoryOrFileInfo(
            (PFILE_NAMES_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      default:
        break;
    }
  } __finally {
  }

  return FLT_POSTOP_FINISHED_PROCESSING;
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

bool IsHiddenDir(PUNICODE_STRING dir, PUNICODE_STRING file) {
  KdPrint(
      ("******[IsHiddenDir]dir is %wZ---file is %wZ**********\n", dir, file));
  UNICODE_STRING desDir = RTL_CONSTANT_STRING(
      L"\\Device\\HarddiskVolume3\\Users\\WDKRemoteUser\\Desktop");
  UNICODE_STRING desFile = RTL_CONSTANT_STRING(L"hidden folder");
  // 隐藏此文件夹下所有文件和文件夹
  UNICODE_STRING desDirFile = RTL_CONSTANT_STRING(
      L"\\Device\\HarddiskVolume3\\Users\\WDKRemoteUser\\Desktop\\hidden "
      L"folder");
  UNICODE_STRING excludeFile1 = RTL_CONSTANT_STRING(L".");
  UNICODE_STRING excludeFile2 = RTL_CONSTANT_STRING(L"..");

  return (RtlEqualUnicodeString(&desDir, dir, true) &&
          RtlEqualUnicodeString(&desFile, file, true)) ||
         (RtlEqualUnicodeString(&desDirFile, dir, true) &&
          !RtlEqualUnicodeString(&excludeFile1, file, true) &&
          !RtlEqualUnicodeString(&excludeFile2, file, true));
}