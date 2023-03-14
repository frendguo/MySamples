#pragma once
#include <fltKernel.h>
#include <ntddk.h>

template <typename T>
NTSTATUS CleanDirectoryOrFileInfo(T* info, PFLT_FILE_NAME_INFORMATION fltname) {
  typedef T* TP;
  UNICODE_STRING fileName{};
  TP preInfo = NULL, nextInfo = NULL;
  NTSTATUS status = STATUS_SUCCESS;
  bool retn = false;
  int moveLength, offset = 0;

  BOOLEAN search = true;
  do {
    fileName.Buffer = info->FileName;
    fileName.Length = (USHORT)info->FileNameLength;
    fileName.MaximumLength = (USHORT)info->FileNameLength;

    if (IsHiddenDir(&fltname->Name, &fileName)) {
      KdPrint(("\n---IsHiddenTarget!---\n\n"));
      if (preInfo == NULL) {
        if (info->NextEntryOffset == 0) {
          status = STATUS_NO_MORE_ENTRIES;
          retn = true;
        } else {
          nextInfo = (TP)((PUCHAR)info + info->NextEntryOffset);
          moveLength = 0;
          while (nextInfo->NextEntryOffset != 0) {
            // 这里把所有的 entry 的偏移量加起来
            moveLength += nextInfo->NextEntryOffset;
            nextInfo = (TP)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
          }

          // 这里加上一个 FILE_ID_BOTH_DIR_INFORMATION 的 size
          // FileName 字段是 FILE_ID_BOTH_DIR_INFORMATION
          // 中最后一个字段，而且结构中只存储了 FileName 的头 还需要加上
          // FileNameLength
          moveLength += FIELD_OFFSET(T, FileName) +
                        nextInfo->FileNameLength;
          KdPrint(
              ("\n-----RtlMoveMemory---dir is %wZ----filename is %wZ-----\n\n",
               fltname->Name, fileName));
          RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);
        }
      } else {
        if (info->NextEntryOffset == 0) {
          preInfo->NextEntryOffset = 0;
          status = STATUS_SUCCESS;
          retn = true;
        } else {
          preInfo->NextEntryOffset += info->NextEntryOffset;
          offset = info->NextEntryOffset;
        }

        KdPrint(
            ("\n-----RtlFillMemory---dir is %wZ----filename is %wZ-----\n\n",
             fltname->Name, fileName));
        RtlFillMemory(info, sizeof(T), 0);
      }

      if (retn) {
        return status;
      }

      info = (TP)((PUCHAR)info + info->NextEntryOffset);
      continue;
    }

    offset = info->NextEntryOffset;
    preInfo = info;
    info = (TP)((PUCHAR)info + info->NextEntryOffset);
    if (offset == 0) {
      search = false;
    }
  } while (search);

  return status;
}