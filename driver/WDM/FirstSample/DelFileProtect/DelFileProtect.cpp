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

FLT_PREOP_CALLBACK_STATUS DirectoryControlPreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

FLT_POSTOP_CALLBACK_STATUS DirectoryControlPostCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags);
NTSTATUS CleanFileBothDirectoryInfomation(PFILE_BOTH_DIR_INFORMATION info,
                                          PFLT_FILE_NAME_INFORMATION fltname);
NTSTATUS CleanFileDirectoryInfomation(PFILE_DIRECTORY_INFORMATION info,
                                      PFLT_FILE_NAME_INFORMATION fltname);
NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info,
                                           PFLT_FILE_NAME_INFORMATION fltname);
NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info,
                                   PFLT_FILE_NAME_INFORMATION fltname);

NTSTATUS CleanFileIdBothDirectoryInformation(
    PFILE_ID_BOTH_DIR_INFORMATION info,
    PFLT_FILE_NAME_INFORMATION fltname);

NTSTATUS CleanFileIdFullDirectoryInformation(
    PFILE_ID_FULL_DIR_INFORMATION info,
    PFLT_FILE_NAME_INFORMATION fltname);

bool IsPreventProcess(PEPROCESS process);

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
        KdPrint(("------FileBothDirectoryInformation------\n"));
        CleanFileBothDirectoryInfomation(
            (PFILE_BOTH_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileDirectoryInformation:
        KdPrint(("-----FileDirectoryInformation-----\n"));
        CleanFileDirectoryInfomation(
            (PFILE_DIRECTORY_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileFullDirectoryInformation:
        KdPrint(("-----FileFullDirectoryInformation-----\n"));
        CleanFileFullDirectoryInformation(
            (PFILE_FULL_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl
                .QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileIdBothDirectoryInformation:
        KdPrint(("-----FileIdBothDirectoryInformation-----\n"));
        CleanFileIdBothDirectoryInformation(
            (PFILE_ID_BOTH_DIR_INFORMATION)Data->Iopb->Parameters
                .DirectoryControl.QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileIdFullDirectoryInformation:
        KdPrint(("-----FileIdFullDirectoryInformation-----\n"));
        CleanFileIdFullDirectoryInformation(
            (PFILE_ID_FULL_DIR_INFORMATION)Data->Iopb->Parameters
                .DirectoryControl.QueryDirectory.DirectoryBuffer,
            fltName);
        break;
      case FileNamesInformation:
        // 获取特定文件夹中的文件列表，fltname 中就是文件夹名称
        KdPrint(("-----FileNamesInformation-----\n"));
        CleanFileNamesInformation(
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

NTSTATUS CleanFileBothDirectoryInfomation(PFILE_BOTH_DIR_INFORMATION info,
                                          PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING desDir = RTL_CONSTANT_STRING(
      L"\\Device\\HarddiskVolume3\\Users\\WDKRemoteUser\\Desktop\\hidden "
      L"folder");

  UNICODE_STRING directoryName{};
  directoryName.Buffer = info->FileName;
  directoryName.Length = (USHORT)info->FileNameLength;
  directoryName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---directoryname is %wZ---", directoryName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  if (RtlEqualUnicodeString(&desDir, &directoryName, false)) {
    RtlFillMemory(info, sizeof(PFILE_BOTH_DIR_INFORMATION), 0);
  }

  return STATUS_SUCCESS;
}

NTSTATUS CleanFileDirectoryInfomation(PFILE_DIRECTORY_INFORMATION info,
                                      PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING dirName{};
  dirName.Buffer = info->FileName;
  dirName.Length = (USHORT)info->FileNameLength;
  dirName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---dirname is %wZ---", dirName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  return STATUS_SUCCESS;
}

NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info,
                                           PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING dirName{};
  dirName.Buffer = info->FileName;
  dirName.Length = (USHORT)info->FileNameLength;
  dirName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---dirname is %wZ---", dirName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdBothDirectoryInformation(
    PFILE_ID_BOTH_DIR_INFORMATION info,
    PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING dirName{};
  dirName.Buffer = info->FileName;
  dirName.Length = (USHORT)info->FileNameLength;
  dirName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---dirname is %wZ---", dirName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  UNICODE_STRING desDir = RTL_CONSTANT_STRING(
      L"\\Device\\HarddiskVolume3\\Users\\WDKRemoteUser\\Desktop\\hidden "
      L"folder");

  if (RtlEqualUnicodeString(&desDir, &fltname->Name, false)) {
    KdPrint(("\n*******FileIdBothDirectoryInformation:::ResetFolderContent****\n"));
    RtlFillMemory(info, sizeof(PFILE_NAMES_INFORMATION), 0);
  }

  return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdFullDirectoryInformation(
    PFILE_ID_FULL_DIR_INFORMATION info,
    PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING dirName{};
  dirName.Buffer = info->FileName;
  dirName.Length = (USHORT)info->FileNameLength;
  dirName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---dirname is %wZ---", dirName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  return STATUS_SUCCESS;
}

NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info,
                                   PFLT_FILE_NAME_INFORMATION fltname) {
  UNICODE_STRING dirName{};
  dirName.Buffer = info->FileName;
  dirName.Length = (USHORT)info->FileNameLength;
  dirName.MaximumLength = (USHORT)info->FileNameLength;

  KdPrint(("---dirname is %wZ---", dirName));
  KdPrint(("---fltname is %wZ---\n", fltname->Name));

  UNICODE_STRING desDir = RTL_CONSTANT_STRING(
      L"\\Device\\HarddiskVolume3\\Users\\WDKRemoteUser\\Desktop\\hidden "
      L"folder");

  if (RtlEqualUnicodeString(&desDir, &fltname->Name, false)) {
    KdPrint(("\n*******FileNamesInformation:::ResetFolderContent****\n"));
    RtlFillMemory(info, sizeof(PFILE_NAMES_INFORMATION), 0);
  }

  return STATUS_SUCCESS;
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