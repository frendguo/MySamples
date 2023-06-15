#include <ntifs.h>
#include <ntddk.h>

#include "UninstallPreventCommon.h"

typedef NTSTATUS(*PsSuspendProcess)(PEPROCESS Process);
typedef NTSTATUS(*PsResumeProcess)(PEPROCESS Process);

UNICODE_STRING g_unisntallStr;

extern "C" NTKERNELAPI
UCHAR * PsGetProcessImageFileName(__in PEPROCESS Process);
PsSuspendProcess fPsSuspendProcess;
PsResumeProcess fPsResumeProcess;

void OnProcessCreateCloseNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
    UNREFERENCED_PARAMETER(ProcessId);

    // ����ǽ����˳���ֱ�ӷ���
    if (!CreateInfo) {
        return;
    }
    // SeLocateProcessImageName(Process, &imageName); // ͨ���˷�ʽ��ȡ����·��
    UCHAR* procName = PsGetProcessImageFileName(Process);
    if (procName == nullptr) {
        return;
    }

    ANSI_STRING ansiProcName;
    UNICODE_STRING unicodeProcName;
    RtlInitAnsiString(&ansiProcName, (PCSZ)procName);
    auto status = RtlAnsiStringToUnicodeString(&unicodeProcName, &ansiProcName, TRUE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Convert ansi_string to unicode_string fail. \n"));
        return;
    }

    KdPrint(("-----[%s] is created.\n", procName));
    if (RtlEqualUnicodeString(&g_unisntallStr, &unicodeProcName, false)) {
        // ���̾��� uninstall.exe
        // �������
        status = fPsSuspendProcess(Process);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Faild to suspend process. (0X%08X)\n", status));
            return;
        }

        // todo�� ����ɹ��󣬷�����Ϣ��Ӧ�ò�
    }
}

NTSTATUS
UninstallPreventDeviceControl(
    _In_ _DEVICE_OBJECT* DeviceObject,
    _Inout_ _IRP* Irp
) {
    UNREFERENCED_PARAMETER(DeviceObject);

    auto stack = IoGetCurrentIrpStackLocation(Irp);
    auto status = STATUS_SUCCESS;
    ULONG processId = 0;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_UNINSTALL_RESUME_PROCESS:
        if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        processId = *(ULONG*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
        if (processId == 0) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PEPROCESS process;
        status = PsLookupProcessByProcessId((HANDLE)processId, &process);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Fail to lookup process. (0X%08X)\n", status));
            break;
        }

        status = fPsResumeProcess(process);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Fail to resume process. (0X%08X)\n", status));
            break;
        }
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
UninstallPreventCreateClose(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
) {
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT driverObject) {
    PsSetCreateProcessNotifyRoutineEx(OnProcessCreateCloseNotify, true);
    UNICODE_STRING symbolName = RTL_CONSTANT_STRING(L"\\??\\UninstallPrevent");
    IoDeleteSymbolicLink(&symbolName);
    IoDeleteDevice(driverObject->DeviceObject);
}

void DriverInit() {
    g_unisntallStr = RTL_CONSTANT_STRING(L"Uninstall.exe");

    // ��ȡ pssuspendprocess��psresumeprocess ������ַ
    UNICODE_STRING psSuspendProcessName = RTL_CONSTANT_STRING(L"PsSuspendProcess");
    UNICODE_STRING psResumeProcessName = RTL_CONSTANT_STRING(L"PsResumeProcess");
    fPsSuspendProcess = (PsSuspendProcess)MmGetSystemRoutineAddress(&psSuspendProcessName);
    fPsResumeProcess = (PsResumeProcess)MmGetSystemRoutineAddress(&psResumeProcessName);
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registerPath) {
    UNREFERENCED_PARAMETER(registerPath);

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\UninstallPrevent");
    UNICODE_STRING symbolName = RTL_CONSTANT_STRING(L"\\??\\UninstallPrevent");

    KdPrint(("--DriverEntry----\n"));
    do
    {
        status = IoCreateDevice(driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &deviceObject);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Error: fail to create device(code=0x%08X).", status));
            break;
        }
        status = IoCreateSymbolicLink(&symbolName, &deviceName);
        if (!NT_SUCCESS(status)) {
            IoDeleteDevice(deviceObject);
            KdPrint(("Error: fail to create symbollink(code=0x%08X).", status));
            break;
        }

        // ���� unload �ص�
        driverObject->DriverUnload = DriverUnload;

        // ���÷ַ�����
        driverObject->MajorFunction[IRP_MJ_CREATE] = UninstallPreventCreateClose;
        driverObject->MajorFunction[IRP_MJ_CLOSE] = UninstallPreventCreateClose;
        driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UninstallPreventDeviceControl;

        // ���ý��̴����ص�
        // ���� true ��ɾ���ص���false ����ӻص�
        status = PsSetCreateProcessNotifyRoutineEx(OnProcessCreateCloseNotify, false);

        // ��� status �Ĵ�����
        if (!NT_SUCCESS(status)) {
            KdPrint(("Error: fail to set process create notify(code=0x%08X).", status));
            break;
        }
    } while (false);

    if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&symbolName);
        if (deviceObject) {
            IoDeleteDevice(deviceObject);
        }

        return status;
    }

    DriverInit();

    return status;
}

