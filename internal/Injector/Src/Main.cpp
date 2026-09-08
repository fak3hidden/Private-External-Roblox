#include "Bypass/IBypass.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

static DWORD FindPid(const wchar_t* exe) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exe) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static std::string ClientVersion(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return "Unknown (handle denied)";
    wchar_t path[MAX_PATH];
    DWORD sz = MAX_PATH;
    std::string ver = "Unknown";
    if (QueryFullProcessImageNameW(h, 0, path, &sz)) {
        std::wstring p(path);
        size_t end = p.find_last_of(L"\\/");
        if (end != std::wstring::npos) {
            size_t start = p.find_last_of(L"\\/", end - 1);
            std::wstring folder = (start == std::wstring::npos) ? p.substr(0, end) : p.substr(start + 1, end - start - 1);
            ver.assign(folder.begin(), folder.end());
        }
    }
    CloseHandle(h);
    return ver;
}

int main(int argc, char** argv) {
    std::cout << "--- UCR Internal Injector (skeleton) ---\n\n";
    std::string dll = (argc > 1) ? argv[1] : "UCRCore.dll";

    DWORD pid = FindPid(L"RobloxPlayerBeta.exe");
    if (!pid) {
        std::cout << "[!] RobloxPlayerBeta.exe not running.\n";
        return 1;
    }
    std::cout << "[+] PID: " << pid << "\n";
    std::cout << "[*] Client: " << ClientVersion(pid) << "\n";
    std::cout << "[*] DLL: " << dll << "\n\n";

    auto providers = GetProviders();
    for (auto& p : providers) {
        std::string why;
        std::cout << "[*] Provider: " << p->Name() << " ... ";
        if (!p->IsAvailable(why)) {
            std::cout << std::string("UNAVAILABLE: ") + why + "\n";
            continue;
        }
        InjectResult r = p->Inject(pid, dll);
        std::cout << (r.ok ? std::string("OK\n") : (std::string("FAILED: ") + r.error + "\n"));
        if (r.ok) return 0;
    }
    std::cout << "\n[!] Injection did not happen. See internal/README.md (bypass interface).\n";
    return 2;
}
