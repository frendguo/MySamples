#pragma once

NTSTATUS ResolveSymbolicLink(PUNICODE_STRING Link, PUNICODE_STRING Resolved);
NTSTATUS NormalizeDevicePath(PUNICODE_STRING Path,
                             PUNICODE_STRING NormalizedPath);