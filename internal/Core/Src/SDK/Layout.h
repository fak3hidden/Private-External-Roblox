#pragma once
#include <string>
#include <vector>
#include "../Mem.h"

// Minimal layout subset for in-process use. SYNC SOURCE: the external's Offsets.h
// (same client build => same layout). Only what the skeleton needs is mirrored here.
namespace Layout {
    inline constexpr uintptr_t FakeDataModelPtr = 0x8d22868;
    inline constexpr uintptr_t RealDataModel = 0x1f8;
    inline constexpr uintptr_t ChildrenStart = 0x78;
    inline constexpr uintptr_t ClassDescriptor = 0x18;
    inline constexpr uintptr_t ClassName = 0x8;
    // NOTE: Name may need the runtime sweep (see external Main.cpp) once the
    // skeleton grows past class-only lookups. Nothing here reads names yet.
    inline constexpr uintptr_t Name = 0x98;
    inline constexpr uintptr_t Parent = 0x68;
    inline constexpr uintptr_t Player_LocalPlayer = 0x130;
    inline constexpr uintptr_t Player_ModelInstance = 0x298;
    inline constexpr uintptr_t BasePart_Primitive = 0x188;
    inline constexpr uintptr_t Prim_Position = 0xec;
    inline constexpr uintptr_t Prim_LinearVel = 0xf8;
    inline constexpr uintptr_t Hum_Walkspeed = 0x1d0;
    inline constexpr uintptr_t Hum_WalkspeedCheck = 0x3bc;
}

namespace ISDK {
    using Addr = uintptr_t;

    inline std::string ReadStdString(Addr strObj) {
        // MSVC64 std::string: { ptr|buf[16] @0, size @0x10, cap @0x18 }
        if (!strObj) return "";
        int32_t cap = Mem::Read<int32_t>(strObj + 0x18, 0);
        Addr data = strObj;
        if (cap >= 16) data = Mem::Read<Addr>(strObj, 0);
        if (!data) return "";
        char buf[256] = {};
        for (int i = 0; i < 255; ++i) {
            char c = Mem::Read<char>(data + i, 0);
            if (!c) break;
            buf[i] = c;
        }
        return std::string(buf);
    }

    inline std::string GetClass(Addr inst) {
        if (!inst) return "";
        Addr desc = Mem::Read<Addr>(inst + Layout::ClassDescriptor, 0);
        if (!desc) return "";
        Addr np = Mem::Read<Addr>(desc + Layout::ClassName, 0);
        if (!np) return "";
        return ReadStdString(np);
    }

    inline std::vector<Addr> GetChildren(Addr inst) {
        std::vector<Addr> out;
        if (!inst) return out;
        Addr vec = Mem::Read<Addr>(inst + Layout::ChildrenStart, 0);
        if (!vec) return out;
        Addr cur = Mem::Read<Addr>(vec, 0);
        Addr end = Mem::Read<Addr>(vec + 0x8, 0);
        if (!cur || end <= cur) return out;
        size_t count = (size_t)(end - cur) / 0x10;
        if (count > 200000) return out;
        for (size_t i = 0; i < count; ++i) {
            Addr e = Mem::Read<Addr>(cur + i * 0x10, 0);
            if (e) out.push_back(e);
        }
        return out;
    }

    inline Addr FindChildByClass(Addr inst, const char* cls) {
        for (Addr c : GetChildren(inst)) {
            if (GetClass(c) == cls) return c;
        }
        return 0;
    }

    inline Addr ResolveDataModel() {
        Addr base = Mem::ModuleBase();
        if (!base) return 0;
        Addr fake = Mem::Read<Addr>(base + Layout::FakeDataModelPtr, 0);
        if (!fake) return 0;
        return Mem::Read<Addr>(fake + Layout::RealDataModel, 0);
    }
}
