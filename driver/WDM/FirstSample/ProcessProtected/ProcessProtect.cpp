#include "ProcessProtect.h"
#include <ntddk.h>
#include <ntifs.h>

#include "AutoLock.h"

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext,
                                          POB_PRE_OPERATION_INFORMATION info);

Globals g_Data;

void DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                                PUNICODE_STRING RegisterPath) {
  UNREFERENCED_PARAMETER(RegisterPath);

  OB_OPERATION_REGISTRATION operations[] = {
      {// PsProcessType、PsThreadType和ExDesktopObjectType
       PsProcessType,
       // OB_OPERATION_HANDLE_CREATE指对用户模式函数CreateProcess、OpenProcess、CreateThread、OpenThread、CreateDesktop、OpenDesktop和针对这些对象类型的相似函数的调用。
       // OB_OPERATION_HANDLE_DUPLICATE指这些对象类型的句柄复制操作
       OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
       OnPreOpenProcess, NULL},
  };

  OB_CALLBACK_REGISTRATION reg = {OB_FLT_REGISTRATION_VERSION, 1,
                                  RTL_CONSTANT_STRING(L"12322.4432221"),
                                  nullptr, operations};

  auto status = STATUS_SUCCESS;
  UNICODE_STRING deviceName =
      RTL_CONSTANT_STRING(L"\\Device\\" PROCESS_PROTECT_NAME);
  UNICODE_STRING symbolName =
      RTL_CONSTANT_STRING(L"\\??\\" PROCESS_PROTECT_NAME);
  PDEVICE_OBJECT DeviceObject = nullptr;

  g_Data.Init();

  do {
    status = ObRegisterCallbacks(&reg, &g_Data.RegHandle);
    if (!NT_SUCCESS(status)) {
      KdPrint((DRIVER_PREFIX "Failed to register callback (status=%08X).\n",
               status));
      break;
    }

    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN,
                            0, false, &DeviceObject);
    if (!NT_SUCCESS(status)) {
      KdPrint(
          (DRIVER_PREFIX "Failed to create deivce (status=%08X).\n", status));
      break;
    }

    status = IoCreateSymbolicLink(&symbolName, &deviceName);
    if (!NT_SUCCESS(status)) {
      KdPrint((DRIVER_PREFIX "Failed to create symbol link (status=%08X).\n",
               status));
      break;
    }
  } while (false);

  if (!NT_SUCCESS(status)) {
    if (g_Data.RegHandle) {
      ObUnRegisterCallbacks(g_Data.RegHandle);
    }

    if (DeviceObject) {
      IoDeleteDevice(DeviceObject);
    }
  }

  DriverObject->DriverUnload = DriverUnload;
  DriverObject->MajorFunction[IRP_MJ_CREATE] =
      DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateOrClose;

  return status;
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
  ObUnRegisterCallbacks(g_Data.RegHandle);

  UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" PROCESS_PROTECT_NAME);

  IoDeleteDevice(DriverObject->DeviceObject);
  IoDeleteSymbolicLink(&symName);
}

NTSTATUS DriverCreateOrClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);

  auto status = STATUS_SUCCESS;
  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = 0;

  return status;
}

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext,
                                          POB_PRE_OPERATION_INFORMATION info) {
  auto status = OB_PREOP_SUCCESS;

  UNREFERENCED_PARAMETER(RegistrationContext);

  if (info->KernelHandle) {
    // 内核状态下操作，这里直接忽略
    return status;
  }

  UNICODE_STRING processName = RTL_CONSTANT_STRING(L"notepad.exe");

  auto process = (PEPROCESS)info->Object;
  PUNICODE_STRING imageName;
  auto s = SeLocateProcessImageName(process, &imageName);
  if (!NT_SUCCESS(s)) {
    KdPrint(("----Failed to get image name.\n"));
    return status;
  }

  if (imageName->Length < 1) {
    KdPrint(("image paht is empty.\n"));
    return status;
  }

  // KdPrint(("-------%ws\n", imageName->Buffer));

  AutoLock<FastMutex> locker(g_Data.Lock);
  if (wcsstr(imageName->Buffer, processName.Buffer) != NULL) {
    KdPrint(("*----DesiredAccess=(%08X),",
             info->Parameters->CreateHandleInformation.DesiredAccess));
    info->Parameters->CreateHandleInformation.DesiredAccess &=
        ~PROCESS_TERMINATE;
    KdPrint(("*----DesiredAccess2=(%08X)------*\n",
             info->Parameters->CreateHandleInformation.DesiredAccess));
  }

  return status;
}