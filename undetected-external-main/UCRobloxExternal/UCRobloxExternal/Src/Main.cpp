#include "Memory/Communication.h"
#include "Game/Offsets/Offsets.h"
#include "Game/SDK/SDK.h"
#include "Game/W2S/W2S.h"
#include "Render/Render.h"
#include "Core/Globals/Globals.h"
#include "Core/Cache/Cache.h"
#include "Core/Features/Visuals/Visuals.h"
#include "Core/Features/Aimbot/Aimbot.h"
#include "Core/Features/Flight/Flight.h"
#include "Core/Features/Explorer/Explorer.h"
#include "Core/Features/World/World.h"
#include "Core/Features/Movement/Movement.h"
#include "Core/Features/Rage/Rage.h"
#include "Core/Config/Config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <atomic>
#include <filesystem>
#include <random>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstdio>
#include <vector>
#include <utility>

std::atomic<bool> running(true);
std::atomic<bool> gameAttached(false);

// --- Helper Functions ---
bool IsGameRunning(const wchar_t* windowTitle) {
    HWND hwnd = FindWindowW(NULL, windowTitle);
    return hwnd != NULL;
}

bool RescanPointers(uintptr_t baseAddr) {
    try {
        auto fakeDataModelAddr = baseAddr + Offsets::FakeDataModel::Pointer;
        auto fakeDataModel = Coms->ReadMemory<uintptr_t>(fakeDataModelAddr);
        if (!fakeDataModel) return false;

        auto dataModelAddr = fakeDataModel + Offsets::FakeDataModel::RealDataModel;
        auto dataModelPtr = Coms->ReadMemory<uintptr_t>(dataModelAddr);
        if (!dataModelPtr) return false;

        auto visualEngineAddr = baseAddr + Offsets::VisualEngine::Pointer;
        auto visualEngine = Coms->ReadMemory<uintptr_t>(visualEngineAddr);
        if (!visualEngine) return false;

        Globals::dataModel = RBX::RbxInstance(dataModelPtr);
        Globals::renderEngine = RBX::RenderEngine(visualEngine);
        Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
        Globals::players = Globals::dataModel.FindChildByClass("Players");
        Globals::camera = Globals::workspace.FindChildByClass("Camera");

        if (Globals::players.Addr != 0) {
            auto locPlr = Coms->ReadMemory<uintptr_t>(Globals::players.Addr + Offsets::Player::LocalPlayer);
            Globals::localPlayer = RBX::RbxInstance(locPlr);
        }

        // --- Auto-detect Instance::Name (offset x shape sweep) ---
        // Reader is proven (class names resolve); only the Instance-name path varies.
        // Shapes: 0=deref *(C), 1=inline (C), 2=container *(C)+S, 3=dbl-deref *(*(C)+S).
        {
            auto readNameAs = [&](uintptr_t inst, uintptr_t cand, int mode, uintptr_t sub) -> std::string {
                if (inst == 0) return "";
                if (mode == 1) return Coms->ReadGameString(inst + cand);
                uintptr_t c = Coms->ReadMemory<uintptr_t>(inst + cand);
                if (c == 0) return "";
                if (mode == 2) return Coms->ReadGameString(c + sub);
                if (mode == 3) {
                    uintptr_t s = Coms->ReadMemory<uintptr_t>(c + sub);
                    if (s == 0) return "";
                    return Coms->ReadGameString(s);
                }
                return Coms->ReadGameString(c);
            };

            auto votesServices = [&](uintptr_t cand, int mode, uintptr_t sub,
                                     std::string& oDM, std::string& oWS, std::string& oPL) -> int {
                oDM = readNameAs(Globals::dataModel.Addr, cand, mode, sub);
                oWS = readNameAs(Globals::workspace.Addr, cand, mode, sub);
                oPL = readNameAs(Globals::players.Addr, cand, mode, sub);
                int v = 0;
                if (oDM == "Game") v++;
                if (oWS == "Workspace") v++;
                if (oPL == "Players") v++;
                return v;
            };

            const std::pair<int, uintptr_t> tests[] = {
                {0,0},{1,0},
                {2,0x8},{2,0x10},{2,0x18},{2,0x20},{2,0x28},
                {3,0x0},{3,0x8},{3,0x10},{3,0x18},{3,0x20}
            };
            std::vector<uintptr_t> cands = { 0x98, 0x70, 0x8 };
            for (uintptr_t c = 0x10; c <= 0x280; c += 8) {
                if (c == 0x98 || c == 0x70) continue;
                cands.push_back(c);
            }

            uintptr_t winC = 0; int winM = 0; uintptr_t winS = 0; int winV = 0;
            std::string why;

            // Phase 1: service-name oracle (Game / Workspace / Players, need 2/3).
            for (uintptr_t cand : cands) {
                for (auto [mode, sub] : tests) {
                    std::string sDM, sWS, sPL;
                    int v = votesServices(cand, mode, sub, sDM, sWS, sPL);
                    if (v >= 2) { winC = cand; winM = mode; winS = sub; winV = v; why = "services"; break; }
                }
                if (winC) break;
            }

            // Phase 2: local-character part-name oracle (immune to service renames).
            if (!winC) {
                auto localChar = Globals::localPlayer.GetModelRef();
                std::vector<uintptr_t> kidAddrs;
                if (localChar.IsValid() && localChar.GetClass() == "Model") {
                    for (auto& k : localChar.GetChildList()) {
                        kidAddrs.push_back(k.Addr);
                        if (kidAddrs.size() >= 14) break;
                    }
                }
                if (!kidAddrs.empty()) {
                    const char* known[] = {"Head","Torso","Humanoid","HumanoidRootPart","UpperTorso","LowerTorso",
                        "Left Arm","Right Arm","Left Leg","Right Leg","LeftFoot","RightFoot","LeftHand","RightHand",
                        "UpperLeftArm","UpperRightArm","LowerLeftArm","LowerRightArm","UpperLeftLeg","UpperRightLeg",
                        "LowerLeftLeg","LowerRightLeg","LeftUpperArm","RightUpperArm","LeftLowerArm","RightLowerArm",
                        "LeftUpperLeg","RightUpperLeg","LeftLowerLeg","RightLowerLeg"};
                    for (uintptr_t cand : cands) {
                        for (auto [mode, sub] : tests) {
                            int score = 0;
                            for (uintptr_t ka : kidAddrs) {
                                std::string kn = readNameAs(ka, cand, mode, sub);
                                for (auto* want : known) { if (kn == want) { score++; break; } }
                            }
                            if (score >= 3) { winC = cand; winM = mode; winS = sub; winV = score; why = "character"; break; }
                        }
                        if (winC) break;
                    }
                }
            }

            if (winC) {
                Offsets::Instance::Name = winC;
                Offsets::Instance::NameMode = winM;
                Offsets::Instance::NameSub = winS;
                std::cout << "[+] Instance::Name resolved: 0x" << std::hex << winC << std::dec
                          << " mode " << winM << " sub 0x" << std::hex << winS << std::dec
                          << " (" << why << " " << winV << ")\n";
            } else {
                Offsets::Instance::Name = 0x8;
                Offsets::Instance::NameMode = 0;
                Offsets::Instance::NameSub = 0;
                std::cout << "[!] Instance::Name sweep failed (0x8-0x280 x13 shapes, 2 oracles)\n";
            }

        }

        return (Globals::dataModel.Addr != 0 && Globals::workspace.Addr != 0);
    }
    catch (...) {
        return false;
    }
}

// --- Threads ---
void LocalPlayerThread() {
    static bool prevJumpEnabled = false;
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> jitter(-50, 50);

    while (running) {
        if (!gameAttached) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        auto humanoid = character.FindChildByClass("Humanoid");
        if (humanoid.Addr != 0) {
            // NOTE: WalkSpeed is handled by Movement::RunSpeed() (velocity-based, anti-kick).
            if (Vars::Local::jumpEnabled) {
                RBX::ModifyJumpPower(humanoid, Vars::Local::jumpPower);
                prevJumpEnabled = true;
            }
            else if (prevJumpEnabled) {
                RBX::ModifyJumpPower(humanoid, 50.0f);
                prevJumpEnabled = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250 + jitter(rng)));
    }
}

void LogWatcherThread() {
    char* localAppData = nullptr;
    size_t len = 0;

    // Safe environment variable fetching
    if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") != 0 || !localAppData) return;

    std::string logDir = std::string(localAppData) + "\\Roblox\\logs";
    free(localAppData); // Prevent memory leak

    std::string currentLogFile = "";

    while (running) {
        if (!Vars::Misc::autoRescan || !gameAttached) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::string latestLog = "";
        std::filesystem::file_time_type latestTime;
        bool first = true;

        try {
            if (std::filesystem::exists(logDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(logDir)) {
                    if (entry.path().extension() == ".log") {
                        if (first || entry.last_write_time() > latestTime) {
                            latestLog = entry.path().string();
                            latestTime = entry.last_write_time();
                            first = false;
                        }
                    }
                }
            }
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        if (latestLog.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        if (latestLog != currentLogFile) {
            currentLogFile = latestLog;
        }

        std::ifstream logFile(currentLogFile, std::ios::in | std::ios::binary);
        if (!logFile.is_open()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        logFile.seekg(0, std::ios::end);
        std::string line;

        while (running && Vars::Misc::autoRescan && currentLogFile == latestLog) {
            if (std::getline(logFile, line)) {
                if (line.find("! Joining game") != std::string::npos || line.find("Teleporting to") != std::string::npos) {
                    Vars::Misc::rescan = true;
                    NotificationService::Push("Server change detected, Rescanning...", 3.5f);
                    break; // Break inner loop to re-evaluate latest log
                }
            }
            else {
                logFile.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                static int checkCounter = 0;
                if (++checkCounter > 10) {
                    checkCounter = 0;
                    break; // Check for new log file
                }
            }
        }
    }
}

int main() {
    std::cout << "--- Nowhere External ---\n\n";
    std::cout << "[*] Searching for Roblox...\n";

    while (!Coms->Connect(L"RobloxPlayerBeta.exe")) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    auto baseAddr = Coms->GetBase();
    std::cout << "[+] Process ID: " << Coms->GetPID() << "\n";
    std::cout << "[+] Base Address: 0x" << std::hex << baseAddr << std::dec << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!RescanPointers(baseAddr)) {
        std::cout << "[!] Failed to scan initial pointers. Ensure game is fully loaded.\n";
        std::cin.get();
        return -1;
    }

    std::cout << "[+] DataModel: 0x" << std::hex << Globals::dataModel.Addr << std::dec << "\n";
    std::cout << "[+] VisualEngine: 0x" << std::hex << Globals::renderEngine.Addr << std::dec << "\n";
    std::cout << "[+] Workspace: 0x" << std::hex << Globals::workspace.Addr << std::dec << "\n";
    std::cout << "[+] Players: 0x" << std::hex << Globals::players.Addr << std::dec << "\n";
    std::cout << "[+] Camera: 0x" << std::hex << Globals::camera.Addr << std::dec << "\n";
    std::cout << "[+] LocalPlayer: 0x" << std::hex << Globals::localPlayer.Addr << std::dec << "\n\n";

    Config::Load("default");

    if (!Vars::Misc::fastLaunch) {
        wchar_t processPath[MAX_PATH];
        DWORD size = MAX_PATH;
        std::string detectedVersion = "Unknown";
        if (QueryFullProcessImageNameW(Coms->GetHandle(), 0, processPath, &size)) {
            std::filesystem::path path(processPath);
            detectedVersion = path.parent_path().filename().string();
        }

        if (detectedVersion != Offsets::ClientVersion) {
            std::cout << "[!] Version Mismatch Detected!\n";
            std::cout << "[*] Offsets are updated for [" << Offsets::ClientVersion << "] and you are on [" << detectedVersion << "]\n";
            std::cout << "[?] Would you still like to launch the external? (Y/N): ";
            char choice;
            std::cin >> choice;
            if (choice != 'y' && choice != 'Y') {
                return 0;
            }
        }
        else {
            std::cout << "[+] Roblox version matches offsets. Launching...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    OverlayWindow overlay;
    if (!overlay.Initialize()) {
        std::cout << "[!] Failed to initialize overlay\n";
        std::cin.get();
        return -1;
    }

    std::cout << "[+] Overlay initialized\n";
    std::cout << "[*] Press INSERT to toggle menu\n\n";

    std::thread localThread(LocalPlayerThread);
    std::thread logWatcher(LogWatcherThread);

    NotificationService::Push("Injected successfully", 3.0f);
    gameAttached = true;

    // Timing variables
    static auto lastTime = std::chrono::high_resolution_clock::now();
    static int frameCount = 0;
    static int fps = 0;
    static int cacheCounter = 0;

    while (Coms->IsConnected()) {
        if (!IsGameRunning(L"Roblox")) break;

        // Input
        if (GetAsyncKeyState(VK_INSERT) & 1) Vars::menuOpen = !Vars::menuOpen;

        // F8: console debug dump (names / children / character resolution)
        if (GetAsyncKeyState(VK_F8) & 1) {
            std::cout << "\n===== DEBUG DUMP =====\n";
            std::cout << "PlayersSvc: 0x" << std::hex << Globals::players.Addr << std::dec
                      << " children=" << Globals::players.GetChildList().size() << "\n";
            std::cout << "LocalPlayer: 0x" << std::hex << Globals::localPlayer.Addr << std::dec
                      << " name='" << Globals::localPlayer.GetName()
                      << "' class='" << Globals::localPlayer.GetClass() << "'\n";
            uintptr_t lnp = Coms->ReadMemory<uintptr_t>(Globals::localPlayer.Addr + Offsets::Instance::Name);
            std::cout << "LocalPlayer namePtr=0x" << std::hex << lnp << std::dec << "\n";
            if (lnp != 0) {
                char nbuf[48] = {};
                Coms->ReadBuffer(lnp, nbuf, sizeof(nbuf));
                std::cout << " name bytes: ";
                for (int i = 0; i < 48; ++i)
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)nbuf[i] << " ";
                std::cout << std::dec << "\n";
                std::cout << " len@+0x18=" << Coms->ReadMemory<int32_t>(lnp + 0x18) << "\n";
            }
            auto lc = Globals::localPlayer.GetModelRef();
            std::cout << "LocalChar: 0x" << std::hex << lc.Addr << std::dec
                      << " class='" << lc.GetClass() << "'\n";
            if (lc.IsValid()) {
                auto kids = lc.GetChildList();
                std::cout << "LocalChar children=" << kids.size() << "\n";
                int shown = 0;
                for (auto& k : kids) {
                    if (shown++ >= 25) break;
                    std::cout << "  0x" << std::hex << k.Addr << std::dec
                              << " name='" << k.GetName() << "' class='" << k.GetClass() << "'\n";
                }
            }
            std::cout << "localPos=(" << PlayerCache::localPlayerPos.X << "," << PlayerCache::localPlayerPos.Y << "," << PlayerCache::localPlayerPos.Z << ")\n";
            int cshown = 0;
            for (auto& p : PlayerCache::players) {
                if (cshown++ >= 3) break;
                std::cout << "Cached: player=0x" << std::hex << p.playerAddr << " char=0x" << p.characterAddr
                          << " head=0x" << p.headAddr << " root=0x" << p.rootPartAddr << std::dec
                          << " name='" << p.name << "' pos=(" << p.position.X << "," << p.position.Y << "," << p.position.Z << ")\n";
            }
            std::cout << "======================\n";
        }

        // Teleportation/Rescan Handler
        if (Vars::Misc::rescan) {
            gameAttached = false;
            Vars::Misc::rescan = false;

            if (RescanPointers(baseAddr)) {
                NotificationService::Push("Pointers rescanned successfully", 2.0f);
            }
            else {
                NotificationService::Push("Rescan failed, retrying...", 2.0f);
                Vars::Misc::rescan = true; // Try again next frame
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            gameAttached = true;
        }

        // Player Cache (Every 3 frames)
        if (cacheCounter++ % 3 == 0) PlayerCache::UpdatePlayers();

        overlay.BeginFrame();

        // FPS Calculation
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
        if (elapsed >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = currentTime;
        }

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Engine / Matrix (single fetch per frame, shared by all features)
        auto viewMatrix = Globals::renderEngine.GetViewMat();

        // Watermark
        std::string watermark = "Nowhere External | FPS: " + std::to_string(fps);
        ImVec2 textSize = ImGui::CalcTextSize(watermark.c_str());
        ImVec2 watermarkPos = ImVec2(ImGui::GetIO().DisplaySize.x - textSize.x - 10, 10);

        drawList->AddText(ImVec2(watermarkPos.x - 1, watermarkPos.y), IM_COL32(0, 0, 0, 255), watermark.c_str());
        drawList->AddText(ImVec2(watermarkPos.x + 1, watermarkPos.y), IM_COL32(0, 0, 0, 255), watermark.c_str());
        drawList->AddText(ImVec2(watermarkPos.x, watermarkPos.y - 1), IM_COL32(0, 0, 0, 255), watermark.c_str());
        drawList->AddText(ImVec2(watermarkPos.x, watermarkPos.y + 1), IM_COL32(0, 0, 0, 255), watermark.c_str());
        drawList->AddText(watermarkPos, IM_COL32(255, 255, 255, 255), watermark.c_str());

        // FOV Circles
        POINT p;
        GetCursorPos(&p);
        ImVec2 center = ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));

        if (Vars::Aimbot::enabled && Vars::Aimbot::showFOV) {
            drawList->AddCircle(center, Vars::Aimbot::fovRadius, IM_COL32(0, 0, 0, 255), 64, 2.0f);
            drawList->AddCircle(center, Vars::Aimbot::fovRadius, IM_COL32(255, 255, 255, 255), 64, 1.0f);
        }

        if (Vars::Aimbot::silentAim && Vars::Aimbot::showSilentFOV) {
            drawList->AddCircle(center, Vars::Aimbot::silentFOV, IM_COL32(0, 0, 0, 255), 64, 2.0f);
            drawList->AddCircle(center, Vars::Aimbot::silentFOV, IM_COL32(255, 100, 100, 255), 64, 1.0f);
        }

        // Custom Crosshair
        if (Vars::ESP::crosshair) {
            float cx = static_cast<float>(p.x);
            float cy = static_cast<float>(p.y);
            float size = Vars::ESP::crosshairSize;
            float gap = Vars::ESP::crosshairGap;
            float thick = Vars::ESP::crosshairThickness;
            ImU32 cCol = IM_COL32(
                (int)(Vars::ESP::crosshairColor[0] * 255),
                (int)(Vars::ESP::crosshairColor[1] * 255),
                (int)(Vars::ESP::crosshairColor[2] * 255),
                (int)(Vars::ESP::crosshairColor[3] * 255));
            ImU32 outlineCol = IM_COL32(0, 0, 0, 255);

            auto drawLine = [&](ImVec2 p1, ImVec2 p2) {
                drawList->AddLine(p1, p2, outlineCol, thick + 2.0f);
                drawList->AddLine(p1, p2, cCol, thick);
                };

            drawLine(ImVec2(cx - gap - size, cy), ImVec2(cx - gap, cy));
            drawLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + size, cy));
            drawLine(ImVec2(cx, cy - gap - size), ImVec2(cx, cy - gap));
            drawLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + size));

            if (Vars::ESP::crosshairDot) {
                drawList->AddCircleFilled(ImVec2(cx, cy), thick + 1.0f, outlineCol);
                drawList->AddCircleFilled(ImVec2(cx, cy), thick, cCol);
            }
        }


        // Feature Execution
        Flight::RunFlight();
        Movement::RunInfiniteJump();
        Movement::RunSpeed();
        Movement::RunNoclip();
        Movement::RunDesync();
        Aimbot::RunAimbot(viewMatrix);
        Aimbot::RunTriggerbot(viewMatrix);
        Rage::RunRage();
        Visuals::RenderESP(drawList, viewMatrix);
        Explorer::RenderExplorer();
        World::RunWorld();

        NotificationService::Render(drawList);

        overlay.RenderMenu();
        overlay.EndFrame();
    }

    // Cleanup
    running = false;
    gameAttached = false;

    if (localThread.joinable()) localThread.join();
    if (logWatcher.joinable()) logWatcher.join();

    overlay.Cleanup();

    std::cout << "[*] Exited safely.\n";
    return 0;
}