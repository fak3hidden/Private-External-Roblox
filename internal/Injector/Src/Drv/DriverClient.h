#pragma once
// Usermode client for UCRDrv. IOCTLs + UCR_REQ layout MUST match Driver/Src/DrvMain.c.
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

#define UCR_IOCTL_BASE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_READ    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_WRITE   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_ALLOC   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_FREE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define UCR_IOCTL_PROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct UCR_REQ {
    uint32_t Pid;
    uint64_t Address;
    uint32_t Size;
    uint32_t Extra;
};
static_assert(sizeof(UCR_REQ) == 24, "UCR_REQ layout must match the driver");

class UCRDriver {
public:
    UCRDriver() = default;
    ~UCRDriver() { Close(); }

    bool Open(std::string& err) {
        Close();
        h_ = CreateFileW(L"\\\\.\\UCRDrv", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h_ == INVALID_HANDLE_VALUE) {
            h_ = nullptr;
            err = LastErr("CreateFile \\\\.\\UCRDrv");
            return false;
        }
        return true;
    }

    void Close() {
        if (h_ && h_ != INVALID_HANDLE_VALUE) CloseHandle(h_);
        h_ = nullptr;
    }

    bool Base(uint32_t pid, uint64_t& out) {
        uint32_t p = pid;
        DWORD ret = 0;
        if (!DeviceIoControl(h_, UCR_IOCTL_BASE, &p, sizeof(p), &out, sizeof(out), &ret, nullptr)) return false;
        return ret == sizeof(out) && out != 0;
    }

    bool Read(uint32_t pid, uint64_t addr, void* out, uint32_t size) {
        UCR_REQ r{ pid, addr, size, 0 };
        DWORD ret = 0;
        if (!DeviceIoControl(h_, UCR_IOCTL_READ, &r, sizeof(r), out, size, &ret, nullptr)) return false;
        return ret == size;
    }

    bool Write(uint32_t pid, uint64_t addr, const void* data, uint32_t size) {
        std::vector<uint8_t> buf(sizeof(UCR_REQ) + size);
        UCR_REQ* r = (UCR_REQ*)buf.data();
        r->Pid = pid;
        r->Address = addr;
        r->Size = size;
        r->Extra = 0;
        memcpy(buf.data() + sizeof(UCR_REQ), data, size);
        uint32_t done = 0;
        DWORD ret = 0;
        if (!DeviceIoControl(h_, UCR_IOCTL_WRITE, buf.data(), (DWORD)buf.size(), &done, sizeof(done), &ret, nullptr)) return false;
        return done == size;
    }

    bool Alloc(uint32_t pid, uint32_t size, uint32_t protect, uint64_t& out) {
        UCR_REQ r{ pid, 0, size, protect };
        DWORD ret = 0;
        if (!DeviceIoControl(h_, UCR_IOCTL_ALLOC, &r, sizeof(r), &out, sizeof(out), &ret, nullptr)) return false;
        return ret == sizeof(out) && out != 0;
    }

    bool Free(uint32_t pid, uint64_t addr) {
        UCR_REQ r{ pid, addr, 0, 0 };
        DWORD ret = 0;
        return DeviceIoControl(h_, UCR_IOCTL_FREE, &r, sizeof(r), nullptr, 0, &ret, nullptr) != 0;
    }

    static std::string LastErr(const char* what) {
        DWORD e = GetLastError();
        char* msg = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, e, 0, (LPSTR)&msg, 0, nullptr);
        std::string s = std::string(what) + " failed (" + std::to_string(e) + ")";
        if (msg) {
            s += ": ";
            s += msg;
            LocalFree(msg);
        }
        return s;
    }

private:
    HANDLE h_ = nullptr;
};
