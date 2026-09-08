#include <windows.h>
#include <string>
#include "Log.h"
#include "Mem.h"
#include "Version.h"
#include "SDK/Layout.h"
#include "Features/WalkSpeed.h"

static DWORD WINAPI WorkerThread(LPVOID) {
    char tmp[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmp);
    FileLog::Instance().Init(std::string(tmp) + "UCRInternal.log");
    FileLog::Instance().Write("[*] UCR internal core loaded (skeleton).");

    std::string ver;
    if (!IsClientSupported(ver)) {
        FileLog::Instance().Write("[!] Unsupported client: " + ver + " - refusing to run.");
        return 1;
    }
    FileLog::Instance().Write("[+] Client supported: " + ver);

    uintptr_t dm = ISDK::ResolveDataModel();
    FileLog::Instance().WriteHex("[+] DataModel", dm);
    if (!dm) {
        FileLog::Instance().Write("[!] DataModel resolve failed.");
        return 1;
    }

    uintptr_t players = ISDK::FindChildByClass(dm, "Players");
    FileLog::Instance().WriteHex("[+] Players", players);
    uintptr_t ws = ISDK::FindChildByClass(dm, "Workspace");
    FileLog::Instance().WriteHex("[+] Workspace", ws);

    uintptr_t localPlayer = 0;
    if (players) {
        localPlayer = Mem::Read<uintptr_t>(players + Layout::Player_LocalPlayer, 0);
        FileLog::Instance().WriteHex("[+] LocalPlayer", localPlayer);
    }

    // Idle loop: demo features tick here. The skeleton touches nothing unless
    // its flag files exist (see Features/WalkSpeed.h).
    int tick = 0;
    for (;;) {
        Demo::TickWalkSpeed(localPlayer);
        // Re-resolve occasionally (teleports / respawns change instances).
        if (++tick % 200 == 0 && players) {
            localPlayer = Mem::Read<uintptr_t>(players + Layout::Player_LocalPlayer, localPlayer);
        }
        Sleep(50);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        HANDLE t = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
