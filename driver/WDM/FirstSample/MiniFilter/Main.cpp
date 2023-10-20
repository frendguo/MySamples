#include <fltKernel.h>
#include <ntddk.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject);
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

FLT_PREOP_CALLBACK_STATUS FileReadPreCallback(PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

FLT_PREOP_CALLBACK_STATUS FileWritePreCallback(PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext);

FLT_POSTOP_CALLBACK_STATUS FileReadPostCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

PFLT_FILTER g_filter;

// error LNK2019: unresolved external symbol FltRegisterFilter
// https://stackoverflow.com/questions/72504346/fltregisterfilter-referenced-in-function-driverentry-in-filter-obj
// $(DDK_LIB_PATH)\fltMgr.lib
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegisterPath) {
    UNREFERENCED_PARAMETER(RegisterPath);
    NTSTATUS status = STATUS_SUCCESS;

    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\MiniFltDemo");
    UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\MiniFltDemo");
    PDEVICE_OBJECT deviceObject;
    do {
        status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0,
            false, &deviceObject);
        if (!NT_SUCCESS(status)) {
            break;
        }

        // 如果不需要与用户模式通信，符号链接就不需要
        status = IoCreateSymbolicLink(&symName, &devName);
        if (!NT_SUCCESS(status)) {
            IoDeleteDevice(deviceObject);
            break;
        }

        // register minifilter

        FLT_OPERATION_REGISTRATION operationReg[] = {
            {IRP_MJ_CREATE, 0, FileCreatePreCallback, NULL},
            {IRP_MJ_READ, 0, FileReadPreCallback, FileReadPostCallback},
            {IRP_MJ_WRITE, 0, FileWritePreCallback, NULL},
            {IRP_MJ_OPERATION_END} };

        FLT_REGISTRATION reg = {
            sizeof(FLT_REGISTRATION),  // Size
            FLT_REGISTRATION_VERSION,  // Version
            FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS |
                FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME,  // Flags
            NULL,                                       // ContextRegistration
            operationReg,                               // OperationRegistration
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

        status = FltRegisterFilter(DriverObject, &reg, &g_filter);
        if (NT_SUCCESS(status)) {
            status = FltStartFiltering(g_filter);
            if (!NT_SUCCESS(status)) {
                FltUnregisterFilter(g_filter);
            }
        }
    } while (false);

    DriverObject->DriverUnload = DriverUnload;

    return status;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    IoDeleteDevice(DriverObject->DeviceObject);
    FltUnregisterFilter(g_filter);
}

NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
    UNREFERENCED_PARAMETER(Flags);
    KdPrint(("---[MiniFilter]FilterUnload----\n"));
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

    KdPrint(("---[MiniFilter]FilterInstanceSetupCallback----\n"));
    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS FileCreatePreCallback(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    FLT_PREOP_CALLBACK_STATUS status = FLT_PREOP_SUCCESS_NO_CALLBACK;
    KdPrint(("---[MiniFilter]FileCreatePreCallback----\n"));
    return status;
}

FLT_PREOP_CALLBACK_STATUS FileReadPreCallback(PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    // FLT_PREOP_SUCCESS_WITH_CALLBACK 才能触发 readpostcallback
    // FLT_PREOP_SUCCESS_NO_CALLBACK 不会触发 readpostcallback

    FLT_PREOP_CALLBACK_STATUS status = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    KdPrint(("---[MiniFilter]FileReadPreCallback----\n"));

    NTSTATUS ntStatus = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = FltParseFileNameInformation(nameInfo);
        if (NT_SUCCESS(status)) {
            // 在这里，你可以访问 nameInfo->Name 等字段来获取你想要的路径或文件名部分
            KdPrint(("-----[MiniFilter]File path: %wZ\n", &nameInfo->Name));
        }
    }

    HANDLE processId = PsGetThreadProcessId(Data->Thread);


    return status;
}

FLT_POSTOP_CALLBACK_STATUS FileReadPostCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
) {
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    FLT_POSTOP_CALLBACK_STATUS status = FLT_POSTOP_FINISHED_PROCESSING;
    KdPrint(("---[MiniFilter]FileReadPostCallback----\n"));

    return status;
}

FLT_PREOP_CALLBACK_STATUS FileWritePreCallback(PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID* CompletionContext) {
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    FLT_PREOP_CALLBACK_STATUS status = FLT_PREOP_SUCCESS_NO_CALLBACK;
    KdPrint(("---[MiniFilter]FileWritePreCallback----\n"));

    return status;
}
