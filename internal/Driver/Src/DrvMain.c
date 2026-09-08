// UCRDrv - minimal kernel helper for the internal track.
// Provides: module base, read/write/alloc/free/protect on a target PID.
// Everything is SEH/API-guarded and length-capped. No hiding, no hooks,
// no kernel patching (PatchGuard territory) - that work comes later, if ever,
// and only with a tested design. A bug here = BSOD: keep changes small.
#include <ntddk.h>
#include "MemOps.h"

#define UCR_DEVICE_NAME  L"\\Device\\UCRDrv"
#define UCR_SYMLINK_NAME L"\\DosDevices\\UCRDrv"

// MUST match Injector/Src/Drv/DriverClient.h
#define UCR_IOCTL_BASE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_READ    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_WRITE   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_ALLOC   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_FREE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_PROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define UCR_MAX_XFER (1024 * 1024)

// Natural MSVC x64 layout (24 bytes). MUST match the usermode struct.
typedef struct _UCR_REQ {
    ULONG Pid;
    ULONGLONG Address;
    ULONG Size;
    ULONG Extra; // protect flags for ALLOC/PROTECT
} UCR_REQ, *PUCR_REQ;

static NTSTATUS UCRCreateClose(PDEVICE_OBJECT d, PIRP i) {
    UNREFERENCED_PARAMETER(d);
    i->IoStatus.Status = STATUS_SUCCESS;
    i->IoStatus.Information = 0;
    IoCompleteRequest(i, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS UCRControl(PDEVICE_OBJECT d, PIRP i) {
    UNREFERENCED_PARAMETER(d);
    PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(i);
    NTSTATUS st = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR info = 0;
    PUCR_REQ req = (PUCR_REQ)i->AssociatedIrp.SystemBuffer;
    ULONG inLen = sp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = sp->Parameters.DeviceIoControl.OutputBufferLength;

    switch (sp->Parameters.DeviceIoControl.IoControlCode) {
    case UCR_IOCTL_BASE: {
        if (inLen < sizeof(ULONG) || outLen < sizeof(ULONGLONG)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        ULONGLONG base = 0;
        st = UCRGetBase(req->Pid, &base);
        if (NT_SUCCESS(st)) { *(ULONGLONG*)req = base; info = sizeof(ULONGLONG); }
        break;
    }
    case UCR_IOCTL_READ: {
        if (inLen < sizeof(UCR_REQ)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        ULONG n = req->Size;
        if (n == 0 || n > UCR_MAX_XFER || outLen < n) { st = STATUS_INVALID_PARAMETER; break; }
        SIZE_T done = 0;
        st = UCRRead(req->Pid, req->Address, req, n, &done); // result reuses system buffer
        info = done;
        break;
    }
    case UCR_IOCTL_WRITE: {
        if (inLen < sizeof(UCR_REQ)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        ULONG n = req->Size;
        if (n == 0 || n > UCR_MAX_XFER || inLen < sizeof(UCR_REQ) + n) { st = STATUS_INVALID_PARAMETER; break; }
        SIZE_T done = 0;
        st = UCRWrite(req->Pid, req->Address, (PUCHAR)req + sizeof(UCR_REQ), n, &done);
        *(ULONG*)req = (ULONG)done;
        info = sizeof(ULONG);
        break;
    }
    case UCR_IOCTL_ALLOC: {
        if (inLen < sizeof(UCR_REQ) || outLen < sizeof(ULONGLONG)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        if (req->Size == 0 || req->Size > 256 * 1024 * 1024) { st = STATUS_INVALID_PARAMETER; break; }
        ULONGLONG addr = 0;
        st = UCRAlloc(req->Pid, req->Size, req->Extra ? req->Extra : PAGE_EXECUTE_READWRITE, &addr);
        if (NT_SUCCESS(st)) { *(ULONGLONG*)req = addr; info = sizeof(ULONGLONG); }
        break;
    }
    case UCR_IOCTL_FREE: {
        if (inLen < sizeof(UCR_REQ)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        st = UCRFree(req->Pid, req->Address);
        break;
    }
    case UCR_IOCTL_PROTECT: {
        if (inLen < sizeof(UCR_REQ) || outLen < sizeof(ULONG)) { st = STATUS_BUFFER_TOO_SMALL; break; }
        if (req->Size == 0 || req->Size > 256 * 1024 * 1024) { st = STATUS_INVALID_PARAMETER; break; }
        ULONG old = 0;
        st = UCRProtect(req->Pid, req->Address, req->Size, req->Extra, &old);
        if (NT_SUCCESS(st)) { *(ULONG*)req = old; info = sizeof(ULONG); }
        break;
    }
    default:
        st = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    i->IoStatus.Status = st;
    i->IoStatus.Information = info;
    IoCompleteRequest(i, IO_NO_INCREMENT);
    return st;
}

static VOID UCRUnload(PDRIVER_OBJECT d) {
    UNICODE_STRING sym;
    RtlInitUnicodeString(&sym, UCR_SYMLINK_NAME);
    IoDeleteSymbolicLink(&sym);
    IoDeleteDevice(d->DeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT d, PUNICODE_STRING r) {
    UNREFERENCED_PARAMETER(r);
    UNICODE_STRING dev, sym;
    RtlInitUnicodeString(&dev, UCR_DEVICE_NAME);
    RtlInitUnicodeString(&sym, UCR_SYMLINK_NAME);

    NTSTATUS st = IoCreateDevice(d, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &d->DeviceObject);
    if (!NT_SUCCESS(st)) return st;

    st = IoCreateSymbolicLink(&sym, &dev);
    if (!NT_SUCCESS(st)) { IoDeleteDevice(d->DeviceObject); return st; }

    d->MajorFunction[IRP_MJ_CREATE] = UCRCreateClose;
    d->MajorFunction[IRP_MJ_CLOSE] = UCRCreateClose;
    d->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UCRControl;
    d->DriverUnload = UCRUnload;
    return STATUS_SUCCESS;
}
