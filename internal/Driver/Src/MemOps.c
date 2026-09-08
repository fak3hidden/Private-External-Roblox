#include "MemOps.h"

static NTSTATUS UCRLookup(ULONG pid, PEPROCESS* out) {
    return PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, out);
}

NTSTATUS UCRGetBase(ULONG pid, PULONGLONG outBase) {
    PEPROCESS proc = NULL;
    NTSTATUS st = UCRLookup(pid, &proc);
    if (!NT_SUCCESS(st)) return st;
    *outBase = (ULONGLONG)PsGetProcessSectionBaseAddress(proc);
    ObDereferenceObject(proc);
    return STATUS_SUCCESS;
}

// MmCopyVirtualMemory is fault-safe: bad addresses fail the call, no BSOD.
NTSTATUS UCRRead(ULONG pid, ULONGLONG src, PVOID dst, SIZE_T size, PSIZE_T done) {
    PEPROCESS proc = NULL;
    NTSTATUS st = UCRLookup(pid, &proc);
    if (!NT_SUCCESS(st)) return st;
    SIZE_T n = 0;
    st = MmCopyVirtualMemory(proc, (PVOID)src, PsGetCurrentProcess(), dst, size, KernelMode, &n);
    if (done) *done = n;
    ObDereferenceObject(proc);
    return st;
}

NTSTATUS UCRWrite(ULONG pid, ULONGLONG dst, PVOID src, SIZE_T size, PSIZE_T done) {
    PEPROCESS proc = NULL;
    NTSTATUS st = UCRLookup(pid, &proc);
    if (!NT_SUCCESS(st)) return st;
    SIZE_T n = 0;
    st = MmCopyVirtualMemory(PsGetCurrentProcess(), src, proc, (PVOID)dst, size, KernelMode, &n);
    if (done) *done = n;
    ObDereferenceObject(proc);
    return st;
}

static NTSTATUS UCROpen(ULONG pid, PHANDLE out) {
    PEPROCESS proc = NULL;
    NTSTATUS st = UCRLookup(pid, &proc);
    if (!NT_SUCCESS(st)) return st;
    st = ObOpenObjectByPointer(proc, OBJ_KERNEL_HANDLE, NULL,
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
        *PsProcessType, KernelMode, out);
    ObDereferenceObject(proc);
    return st;
}

NTSTATUS UCRAlloc(ULONG pid, SIZE_T size, ULONG protect, PULONGLONG outAddr) {
    HANDLE h = NULL;
    NTSTATUS st = UCROpen(pid, &h);
    if (!NT_SUCCESS(st)) return st;
    PVOID addr = NULL;
    SIZE_T sz = size;
    st = ZwAllocateVirtualMemory(h, &addr, 0, &sz, MEM_COMMIT | MEM_RESERVE, protect);
    if (NT_SUCCESS(st)) *outAddr = (ULONGLONG)addr;
    ZwClose(h);
    return st;
}

NTSTATUS UCRFree(ULONG pid, ULONGLONG addr) {
    HANDLE h = NULL;
    NTSTATUS st = UCROpen(pid, &h);
    if (!NT_SUCCESS(st)) return st;
    PVOID a = (PVOID)addr;
    SIZE_T sz = 0;
    st = ZwFreeVirtualMemory(h, &a, &sz, MEM_RELEASE);
    ZwClose(h);
    return st;
}

NTSTATUS UCRProtect(ULONG pid, ULONGLONG addr, SIZE_T size, ULONG protect, PULONG oldProtect) {
    HANDLE h = NULL;
    NTSTATUS st = UCROpen(pid, &h);
    if (!NT_SUCCESS(st)) return st;
    PVOID a = (PVOID)addr;
    SIZE_T sz = size;
    ULONG old = 0;
    st = ZwProtectVirtualMemory(h, &a, &sz, protect, &old);
    if (NT_SUCCESS(st) && oldProtect) *oldProtect = old;
    ZwClose(h);
    return st;
}
