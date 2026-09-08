#pragma once
#include <string>
#include <windows.h>

// Supported client versions. The DLL REFUSES to run on anything else:
// static addresses shift every build, and writing through stale ones corrupts memory.
inline bool IsClientSupported(std::string& outVersion) {
    static const char* kSupported[] = { "version-e7d81637d42c4b23" };
    wchar_t path[MAX_PATH];
    outVersion = "Unknown";
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) return false;
    std::wstring p(path);
    size_t end = p.find_last_of(L"\\/");
    if (end == std::wstring::npos) return false;
    size_t start = p.find_last_of(L"\\/", end - 1);
    std::wstring folder = (start == std::wstring::npos) ? p.substr(0, end) : p.substr(start + 1, end - start - 1);
    outVersion.assign(folder.begin(), folder.end());
    for (auto* s : kSupported) {
        if (outVersion == s) return true;
    }
    return false;
}
