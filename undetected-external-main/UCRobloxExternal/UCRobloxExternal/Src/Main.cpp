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

        return (Globals::dataModel.Addr != 0 && Globals::workspace.Addr != 0);
    }
    catch (...) {
        return false;
    }
}

// --- Threads ---
void LocalPlayerThread() {
    static bool prevSpeedEnabled = false;
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
            if (Vars::Local::speedEnabled) {
                RBX::ModifyWalkspeed(humanoid, Vars::Local::walkSpeed);
                prevSpeedEnabled = true;
            }
            else if (prevSpeedEnabled) {
                RBX::ModifyWalkspeed(humanoid, 16.0f);
                prevSpeedEnabled = false;
            }

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

        // Engine / Matrix (fetched early so the debug watermark can sanity-check it)
        auto viewMatrix = Globals::renderEngine.GetViewMat();

        // Watermark (+ ESP debug readout)
        bool vmOk = (viewMatrix.data[0] != 0.0f || viewMatrix.data[5] != 0.0f ||
                     viewMatrix.data[10] != 0.0f || viewMatrix.data[15] != 0.0f);
        int dbgHeads = 0, dbgProj = 0;
        for (auto& dp : PlayerCache::players) {
            if (!dp.isValid) continue;
            if (dp.headAddr != 0) dbgHeads++;
            RBX::Vec2 ds = W2S::WorldToScreen(dp.position, viewMatrix);
            if (!(ds.X == 0.0f && ds.Y == 0.0f)) dbgProj++;
        }
        std::string watermark = "Nowhere External | FPS: " + std::to_string(fps) +
            " | Players: " + std::to_string(static_cast<int>(PlayerCache::players.size())) +
            "/" + std::to_string(PlayerCache::debugRawCount) +
            " | VM: " + (vmOk ? "ok" : "BAD") +
            " | LP: " + (Globals::localPlayer.Addr != 0 ? "ok" : "none") +
            " | Heads: " + std::to_string(dbgHeads) +
            " | Proj: " + std::to_string(dbgProj);
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