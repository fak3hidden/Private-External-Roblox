#pragma once
#include "../SDK/Layout.h"
#include <string>
#include <fstream>
#include <windows.h>

// Demo feature: in-process dual-write WalkSpeed (same method as the external).
// Driven by %TEMP%\UCRDemoSpeed.txt containing a number (e.g. 100). Write 16
// to restore. This proves end-to-end in-process read/write once injected.
namespace Demo {
    inline void TickWalkSpeed(ISDK::Addr localPlayer) {
        if (!localPlayer) return;
        char tmp[MAX_PATH] = {};
        if (!GetTempPathA(MAX_PATH, tmp)) return;
        std::string flag = std::string(tmp) + "UCRDemoSpeed.txt";
        std::ifstream f(flag);
        float target = 0.0f;
        if (!(f >> target)) return; // no demo file -> do nothing
        if (target < 16.0f) target = 16.0f;
        if (target > 300.0f) target = 300.0f;
        ISDK::Addr ch = Mem::Read<ISDK::Addr>(localPlayer + Layout::Player_ModelInstance, 0);
        if (!ch) return;
        ISDK::Addr hum = ISDK::FindChildByClass(ch, "Humanoid");
        if (!hum) return;
        Mem::Write<float>(hum + Layout::Hum_Walkspeed, target);
        float chk = Mem::Read<float>(hum + Layout::Hum_WalkspeedCheck, 0.0f);
        if (chk >= 0.0f && chk <= 500.0f)
            Mem::Write<float>(hum + Layout::Hum_WalkspeedCheck, target);
    }
}
