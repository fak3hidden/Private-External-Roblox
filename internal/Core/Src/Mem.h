#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <windows.h>

namespace Mem {
    inline uintptr_t ModuleBase(const wchar_t* mod = nullptr) {
        return (uintptr_t)GetModuleHandleW(mod); // NULL = host image
    }

    // SEH-guarded primitives. In-process has no RPM safety net: bad pointers
    // return defaults instead of crashing.
    template <typename T>
    inline T Read(uintptr_t addr, T def = T{}) {
        T out = def;
        __try { memcpy(&out, (void*)addr, sizeof(T)); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return out;
    }

    template <typename T>
    inline bool Write(uintptr_t addr, const T& v) {
        bool ok = false;
        __try { memcpy((void*)addr, &v, sizeof(T)); ok = true; }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return ok;
    }

    // Pattern scan with 'x'/'?' mask over a module's image size.
    inline uintptr_t PatternScan(const wchar_t* mod, const uint8_t* sig, const char* mask) {
        uintptr_t base = ModuleBase(mod);
        if (!base) return 0;
        auto* dos = (IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
        auto* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
        size_t size = nt->OptionalHeader.SizeOfImage;
        size_t len = 0;
        while (mask[len]) ++len;
        if (len == 0) return 0;
        uintptr_t found = 0;
        __try {
            for (size_t i = 0; i + len < size; ++i) {
                bool ok = true;
                for (size_t j = 0; j < len; ++j) {
                    if (mask[j] == 'x' && *(uint8_t*)(base + i + j) != sig[j]) { ok = false; break; }
                }
                if (ok) { found = base + i; break; }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
        return found;
    }

    // Idiom: "48 8B 05 ?? ?? ?? ??" style. Returns 0 on bad input.
    inline uintptr_t PatternScanIda(const wchar_t* mod, const char* ida) {
        std::vector<uint8_t> sig;
        std::string mask;
        const char* p = ida;
        while (*p) {
            while (*p == ' ') ++p;
            if (!*p) break;
            if (*p == '?') {
                sig.push_back(0);
                mask += '?';
                ++p;
                if (*p == '?') ++p;
            }
            else {
                char* end = nullptr;
                long v = strtol(p, &end, 16);
                if (end == p) return 0;
                sig.push_back((uint8_t)v);
                mask += 'x';
                p = end;
            }
        }
        if (sig.empty()) return 0;
        return PatternScan(mod, sig.data(), mask.c_str());
    }
}
