#include <ntddk.h>
#include "Helper.h"

NTSTATUS ResolveSymbolicLink(PUNICODE_STRING Link, PUNICODE_STRING Resolved) {
  OBJECT_ATTRIBUTES attribs;
  HANDLE hsymLink;
  ULONG written;
  NTSTATUS status = STATUS_SUCCESS;

  // Open symlink

  InitializeObjectAttributes(&attribs, Link, OBJ_KERNEL_HANDLE, NULL, NULL);

  status = ZwOpenSymbolicLinkObject(&hsymLink, GENERIC_READ, &attribs);
  if (!NT_SUCCESS(status))
    return status;

  // Query original name

  status = ZwQuerySymbolicLinkObject(hsymLink, Resolved, &written);
  ZwClose(hsymLink);
  if (!NT_SUCCESS(status))
    return status;

  return status;
}

NTSTATUS NormalizeDevicePath(PUNICODE_STRING Path,
    PUNICODE_STRING NormalizedPath) {
  UNICODE_STRING globalPrefix, dvcPrefix, sysrootPrefix;
  NTSTATUS status;

  RtlInitUnicodeString(&globalPrefix, L"\\??\\");
  RtlInitUnicodeString(&dvcPrefix, L"\\Device\\");
  RtlInitUnicodeString(&sysrootPrefix, L"\\SystemRoot\\");

  if (RtlPrefixUnicodeString(&globalPrefix, Path, TRUE)) {
    OBJECT_ATTRIBUTES attribs;
    UNICODE_STRING subPath;
    HANDLE hsymLink;
    ULONG i, written, size;

    subPath.Buffer = (PWCH)((PUCHAR)Path->Buffer + globalPrefix.Length);
    subPath.Length = Path->Length - globalPrefix.Length;

    for (i = 0; i < subPath.Length; i++) {
      if (subPath.Buffer[i] == L'\\') {
        subPath.Length = (USHORT)(i * sizeof(WCHAR));
        break;
      }
    }

    if (subPath.Length == 0)
      return STATUS_INVALID_PARAMETER_1;

    subPath.Buffer = Path->Buffer;
    subPath.Length += globalPrefix.Length;
    subPath.MaximumLength = subPath.Length;

    // Open symlink

    InitializeObjectAttributes(&attribs, &subPath, OBJ_KERNEL_HANDLE, NULL,
                               NULL);

    status = ZwOpenSymbolicLinkObject(&hsymLink, GENERIC_READ, &attribs);
    if (!NT_SUCCESS(status))
      return status;

    // Query original name

    status = ZwQuerySymbolicLinkObject(hsymLink, NormalizedPath, &written);
    ZwClose(hsymLink);
    if (!NT_SUCCESS(status))
      return status;

    // Construct new variable

    size = Path->Length - subPath.Length + NormalizedPath->Length;
    if (size > NormalizedPath->MaximumLength)
      return STATUS_BUFFER_OVERFLOW;

    subPath.Buffer = (PWCH)((PUCHAR)Path->Buffer + subPath.Length);
    subPath.Length = Path->Length - subPath.Length;
    subPath.MaximumLength = subPath.Length;

    status = RtlAppendUnicodeStringToString(NormalizedPath, &subPath);
    if (!NT_SUCCESS(status))
      return status;
  } else if (RtlPrefixUnicodeString(&dvcPrefix, Path, TRUE)) {
    NormalizedPath->Length = 0;
    status = RtlAppendUnicodeStringToString(NormalizedPath, Path);
    if (!NT_SUCCESS(status))
      return status;
  } else if (RtlPrefixUnicodeString(&sysrootPrefix, Path, TRUE)) {
    UNICODE_STRING subPath, resolvedLink, winDir;
    WCHAR buffer[64];
    SHORT i;

    // Open symlink

    subPath.Buffer = sysrootPrefix.Buffer;
    subPath.MaximumLength = subPath.Length =
        sysrootPrefix.Length - sizeof(WCHAR);

    resolvedLink.Buffer = buffer;
    resolvedLink.Length = 0;
    resolvedLink.MaximumLength = sizeof(buffer);

    status = ResolveSymbolicLink(&subPath, &resolvedLink);
    if (!NT_SUCCESS(status))
      return status;

    // \Device\Harddisk0\Partition0\Windows -> \Device\Harddisk0\Partition0
    // Win10: \Device\BootDevice\Windows -> \Device\BootDevice

    winDir.Length = 0;
    for (i = (resolvedLink.Length - sizeof(WCHAR)) / sizeof(WCHAR); i >= 0;
         i--) {
      if (resolvedLink.Buffer[i] == L'\\') {
        winDir.Buffer = resolvedLink.Buffer + i;
        winDir.Length = resolvedLink.Length - (i * sizeof(WCHAR));
        winDir.MaximumLength = winDir.Length;
        resolvedLink.Length = (i * sizeof(WCHAR));
        break;
      }
    }

    // \Device\Harddisk0\Partition0 -> \Device\HarddiskVolume1
    // Win10: \Device\BootDevice -> \Device\HarddiskVolume2

    status = ResolveSymbolicLink(&resolvedLink, NormalizedPath);
    if (!NT_SUCCESS(status))
      return status;

    // Construct new variable

    subPath.Buffer =
        (PWCHAR)((PCHAR)Path->Buffer + sysrootPrefix.Length - sizeof(WCHAR));
    subPath.MaximumLength = subPath.Length =
        Path->Length - sysrootPrefix.Length + sizeof(WCHAR);

    status = RtlAppendUnicodeStringToString(NormalizedPath, &winDir);
    if (!NT_SUCCESS(status))
      return status;

    status = RtlAppendUnicodeStringToString(NormalizedPath, &subPath);
    if (!NT_SUCCESS(status))
      return status;
  } else {
    return STATUS_INVALID_PARAMETER;
  }

  return STATUS_SUCCESS;
}