#pragma once
#include <ntddk.h>

NTSTATUS UCRGetBase(ULONG pid, PULONGLONG outBase);
NTSTATUS UCRRead(ULONG pid, ULONGLONG src, PVOID dst, SIZE_T size, PSIZE_T done);
NTSTATUS UCRWrite(ULONG pid, ULONGLONG dst, PVOID src, SIZE_T size, PSIZE_T done);
NTSTATUS UCRAlloc(ULONG pid, SIZE_T size, ULONG protect, PULONGLONG outAddr);
NTSTATUS UCRFree(ULONG pid, ULONGLONG addr);
NTSTATUS UCRProtect(ULONG pid, ULONGLONG addr, SIZE_T size, ULONG protect, PULONG oldProtect);
