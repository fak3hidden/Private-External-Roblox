#pragma once
// Prevent Windows.h from defining min/max macros which break std::chrono and std::unordered_map
#define NOMINMAX 

#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <unordered_map>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_addons.h"
#include "ImGui/colors.h"

#include "../Core/Vars/Vars.h"
#include "../Core/Config/Config.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==========================================
// Notification System
// ==========================================
struct Notification {
    std::string text;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    float duration;
};

class NotificationService {
private:
    static inline std::vector<Notification> notifications;
    static inline std::mutex notifMutex;

public:
    static void Push(const std::string& text, float duration = 3.0f) {
        std::lock_guard<std::mutex> lock(notifMutex);
        notifications.push_back({ text, std::chrono::steady_clock::now(), duration });
    }

    static void Render(ImDrawList* drawList) {
        std::lock_guard<std::mutex> lock(notifMutex);
        if (notifications.empty()) return;

        auto now = std::chrono::steady_clock::now();
        float screenW = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
        float padding = 15.0f;
        float spacing = 10.0f;
        float startY = 40.0f;

        for (auto it = notifications.begin(); it != notifications.end(); ) {
            float elapsed = std::chrono::duration<float>(now - it->startTime).count();
            if (elapsed > it->duration) {
                it = notifications.erase(it);
                continue;
            }

            float alpha = 1.0f;
            if (elapsed > it->duration - 0.5f) {
                alpha = (it->duration - elapsed) / 0.5f;
            }
            else if (elapsed < 0.2f) {
                alpha = elapsed / 0.2f;
            }

            ImVec2 textSize = ImGui::CalcTextSize(it->text.c_str());
            float height = textSize.y + 16.0f;
            float width = textSize.x + 30.0f;

            ImVec2 pos(screenW - width - padding, startY);

            drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(24, 24, 24, static_cast<int>(255 * alpha)), 5.0f);
            drawList->AddRectFilled(pos + ImVec2(1, 1), ImVec2(pos.x + width - 1, pos.y + height - 1), IM_COL32(0, 0, 0, static_cast<int>(70 * alpha)), 5.0f);
            drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(44, 44, 44, static_cast<int>(255 * alpha)), 5.0f);
            drawList->AddLine(pos + ImVec2(5, 5), ImVec2(pos.x + 5, pos.y + height - 5), IM_COL32(66, 150, 250, static_cast<int>(255 * alpha)), 2.0f);
            drawList->AddText(pos + ImVec2(15.0f, 8.0f), IM_COL32(230, 230, 230, static_cast<int>(255 * alpha)), it->text.c_str());

            startY += height + spacing;
            ++it;
        }
    }
};

// ==========================================
// Overlay Window & D3D11
// ==========================================
LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

class OverlayWindow {
private:
    HWND windowHandle = nullptr;
    WNDCLASSEXW windowClass = { 0 };

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* renderTarget = nullptr;

    void SetupD3D11(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd = { 0 };
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL obtainedLevel;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &swapChain, &d3dDevice, &obtainedLevel, &d3dContext
        );

        if (FAILED(hr)) {
            D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                levels, 2, D3D11_SDK_VERSION, &sd,
                &swapChain, &d3dDevice, &obtainedLevel, &d3dContext
            );
        }

        ID3D11Texture2D* backBuffer = nullptr;
        if (swapChain) {
            swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
            if (backBuffer && d3dDevice) {
                d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
                backBuffer->Release();
            }
        }
    }

    void CleanupD3D11() {
        if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
        if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
        if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
    }

public:
    OverlayWindow() {
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = OverlayWndProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"Nowhere External";
    }

    std::string GetKeyName(int vKey) {
        if (vKey == 0) return "None";
        if (vKey == VK_LBUTTON) return "LMB";
        if (vKey == VK_RBUTTON) return "RMB";
        if (vKey == VK_MBUTTON) return "MMB";
        if (vKey == VK_XBUTTON1) return "MB4";
        if (vKey == VK_XBUTTON2) return "MB5";
        if (vKey == VK_SHIFT) return "Shift";
        if (vKey == VK_LSHIFT) return "LShift";
        if (vKey == VK_RSHIFT) return "RShift";
        if (vKey == VK_CONTROL) return "Ctrl";
        if (vKey == VK_LCONTROL) return "LCtrl";
        if (vKey == VK_RCONTROL) return "RCtrl";
        if (vKey == VK_MENU) return "Alt";
        if (vKey == VK_LMENU) return "LAlt";
        if (vKey == VK_RMENU) return "RAlt";
        if (vKey == VK_SPACE) return "Space";

        char name[128] = { 0 };
        int scanCode = MapVirtualKeyA(vKey, MAPVK_VK_TO_VSC);
        switch (vKey) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= 0x100;
            break;
        }
        if (GetKeyNameTextA(scanCode << 16, name, 128) != 0) {
            return std::string(name);
        }
        return "Key " + std::to_string(vKey);
    }

    struct KeyBindState {
        bool isBinding = false;
        DWORD waitTime = 0;
    };

    void HotkeyButton(const char* label, int* k) {
        ImGui::Text("%s", label);
        ImGui::SameLine(ImGui::GetWindowWidth() - 135);
        ImGui::PushID(label);

        static std::unordered_map<int*, KeyBindState> bindStates;
        KeyBindState& state = bindStates[k];

        std::string buttonText = state.isBinding ? "Press key..." : GetKeyName(*k);
        if (ImGui::Button(buttonText.c_str(), ImVec2(100, 25))) {
            state.isBinding = true;
            state.waitTime = GetTickCount() + 200;
        }

        if (state.isBinding && GetTickCount() > state.waitTime) {
            for (int i = 1; i < 256; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    if (i == VK_ESCAPE) *k = 0;
                    else *k = i;
                    state.isBinding = false;
                    break;
                }
            }
        }
        ImGui::PopID();
    }

    bool Initialize() {
        if (!RegisterClassExW(&windowClass))
            return false;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        windowHandle = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            windowClass.lpszClassName, L"Nowhere External",
            WS_POPUP, 0, 0, screenW, screenH,
            nullptr, nullptr, windowClass.hInstance, nullptr
        );

        if (!windowHandle)
            return false;

        SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);

        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(windowHandle, &margins);

        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);

        SetupD3D11(windowHandle);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.Alpha = 1.0f;
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        style.ChildRounding = 4.0f;
        style.FrameBorderSize = 1.0f;
        style.WindowBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(8, 6);
        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(6, 4);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        ImVec4 accentColor = ImVec4(0.45f, 0.30f, 0.85f, 1.00f);
        ImVec4 accentHover = ImVec4(0.55f, 0.40f, 0.95f, 1.00f);
        ImVec4 accentActive = ImVec4(0.35f, 0.20f, 0.75f, 1.00f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = accentColor;
        colors[ImGuiCol_ButtonActive] = accentActive;

        colors[ImGuiCol_Header] = accentColor;
        colors[ImGuiCol_HeaderHovered] = accentHover;
        colors[ImGuiCol_HeaderActive] = accentActive;

        colors[ImGuiCol_CheckMark] = accentColor;
        colors[ImGuiCol_SliderGrab] = accentColor;
        colors[ImGuiCol_SliderGrabActive] = accentActive;

        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);

        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = accentColor;
        colors[ImGuiCol_SeparatorActive] = accentActive;

        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(d3dDevice, d3dContext);

        return true;
    }

    void BeginFrame() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LONG_PTR exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;
        if (!Vars::menuOpen) {
            exStyle |= WS_EX_TRANSPARENT; // Click-through when menu is closed
        }
        SetWindowLongPtr(windowHandle, GWL_EXSTYLE, exStyle);

        SetWindowDisplayAffinity(windowHandle, Vars::Misc::streamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void RenderMenu() {
        if (!Vars::menuOpen) return;

        ImGuiStyle& Style = ImGui::GetStyle();
        ImGuiIO& Io = ImGui::GetIO(); (void)Io;

        ImGui::SetNextWindowSize(ImVec2(500, 530), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(Io.DisplaySize.x / 2.0f, Io.DisplaySize.y / 2.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        float HeaderHeight = ImGui::GetFontSize() + 6.0f * 2.0f;

        bool MainWindow = ImGui::Begin("main", &Vars::menuOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar(2);

        if (MainWindow) {
            ImVec2 WinPos = ImGui::GetWindowPos();
            ImVec2 WinSize = ImGui::GetWindowSize();
            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            DrawList->AddRectFilled(WinPos, WinPos + WinSize, Menu::Bg);
            DrawList->AddRect(WinPos + ImVec2(1.0f, 1.0f), WinPos + WinSize - ImVec2(1.0f, 1.0f), Menu::Outline);
            DrawList->AddRect(WinPos + ImVec2(2.0f, 2.0f), WinPos + WinSize - ImVec2(2.0f, 2.0f), Menu::Accent);

            float Top = HeaderHeight + Style.WindowBorderSize - 8.0f;
            float Bottom = Style.WindowBorderSize + Style.WindowPadding.y + 1.0f;

            ImVec2 ContentMin = WinPos + ImVec2(Style.WindowBorderSize * 2.0f + Style.WindowPadding.x, Top + Style.WindowPadding.y + 6.0f);
            ImVec2 ContentMax = WinPos + ImVec2(WinSize.x - Style.WindowBorderSize * 2.0f - Style.WindowPadding.x, WinSize.y - Bottom);

            DrawList->AddRectFilled(ContentMin, ContentMax, Menu::InnerBg);
            DrawList->AddRect(ContentMin - ImVec2(1.0f, 1.0f), ContentMax + ImVec2(1.0f, 1.0f), Menu::Outline);
            DrawList->AddRect(ContentMin - ImVec2(2.0f, 2.0f), ContentMax + ImVec2(2.0f, 2.0f), Menu::DarkAccent);

            float TextX = WinPos.x + Style.WindowBorderSize + 8.0f;
            float TextY = WinPos.y + (Top - ImGui::CalcTextSize("Nowhere External").y) * 0.5f + 8.0f;

            ImVec2 TextPos(TextX, TextY);
            Menu::DrawLabelShadow(DrawList, TextPos, Menu::Accent, "Nowhere ");
            TextPos.x += ImGui::CalcTextSize("Nowhere ").x;
            Menu::DrawLabelShadow(DrawList, TextPos, Menu::Text, "External.");

            const char* TabLabels[] = { "Aiming", "Visuals", "World", "Player", "Rage", "Settings" };
            const int TabCount = IM_ARRAYSIZE(TabLabels);
            Menu::Tabs(DrawList, WinPos, TextY, Vars::selectedTab, TabLabels, TabCount, 10.0f, 8.0f);

            ImGui::SetCursorScreenPos(ContentMin);
            ImGui::PushClipRect(ContentMin, ContentMax, true);
            ImGui::BeginGroup();

            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;
            float LeftWidth = (AvailSize.x - Spacing) * 0.5f;
            float RightWidth = AvailSize.x - Spacing - LeftWidth;
            float SidePad = 4.0f;
            float HalfHeight = (AvailSize.y - SidePad * 2.0f) * 0.5f;

            if (Vars::selectedTab == 0) // Aiming
            {
                float startY = ImGui::GetCursorPosY();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                if (Menu::BeginChild("Aimbot", ImVec2(LeftWidth - SidePad, HalfHeight))) {
                    Menu::CheckBox("Enabled", &Vars::Aimbot::enabled);
                    std::vector<const char*> targets = { "Head", "HumanoidRootPart", "Left Arm", "Right Arm", "Left Leg", "Right Leg", "Random" };
                    Menu::Combo("Aim Target", &Vars::Aimbot::aimTarget, targets);
                    Menu::CheckBox("Random Targeting", &Vars::Aimbot::randomTarget);
                    std::vector<const char*> methods = { "Mouse Move", "Camera Rotation" };
                    Menu::Combo("Aim Method", &Vars::Aimbot::aimMethod, methods);

                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Aimbot Keybind", &Vars::Aimbot::aimbotKey);
                    Menu::EndChild();
                }

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);
                float BottomBoxY = HalfHeight - 8;
                if (Menu::BeginChild("Aimbot Settings", ImVec2(LeftWidth - SidePad, BottomBoxY))) {
                    Menu::CheckBox("Team Check", &Vars::Aimbot::teamCheck);
                    Menu::CheckBox("Downed Check", &Vars::Aimbot::downedCheck);
                    Menu::CheckBox("Invis Check", &Vars::Aimbot::invisCheck);
                    Menu::CheckBox("ForceField Check", &Vars::Aimbot::invincibleCheck);
                    Menu::CheckBox("Show FOV Circle", &Vars::Aimbot::showFOV);
                    Menu::SliderFloat("FOV Radius", &Vars::Aimbot::fovRadius, 10.0f, 500.0f, "%.0f");
                    Menu::SliderFloat("Smoothing", &Vars::Aimbot::smoothing, 1.0f, 20.0f, "%.1f");
                    Menu::CheckBox("Prediction", &Vars::Aimbot::prediction);
                    if (Vars::Aimbot::prediction) {
                        // Using original variable names predX and predY
                        Menu::SliderFloat("Prediction X", &Vars::Aimbot::predX, 0.0f, 2.1f, "%.2f");
                        Menu::SliderFloat("Prediction Y", &Vars::Aimbot::predY, 0.0f, 2.1f, "%.2f");
                    }
                    Menu::EndChild();
                }

                ImGui::SameLine(0.0f, Spacing);
                ImGui::SetCursorPosY(startY + SidePad);
                float SilentH = AvailSize.y * 0.40f;
                if (Menu::BeginChild("Silent", ImVec2(RightWidth - SidePad, SilentH))) {
                    Menu::CheckBox("Silent Aim", &Vars::Aimbot::silentAim);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Silent Keybind", &Vars::Aimbot::silentAimbotKey);
                    Menu::CheckBox("Silent Prediction", &Vars::Aimbot::silentPrediction);
                    Menu::CheckBox("Show Silent FOV", &Vars::Aimbot::showSilentFOV);
                    // Using original variable name silentFOV
                    Menu::SliderFloat("Silent FOV", &Vars::Aimbot::silentFOV, 10.0f, 500.0f, "%.0f");
                    Menu::EndChild();
                }

                ImGui::SetCursorPosX(LeftWidth + Spacing);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);
                if (Menu::BeginChild("Triggerbot", ImVec2(RightWidth - SidePad, AvailSize.y - SilentH - SidePad * 3.0f))) {
                    Menu::CheckBox("Triggerbot", &Vars::Triggerbot::enabled);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Trigger Key", &Vars::Triggerbot::key);
                    Menu::SliderFloat("Trigger FOV", &Vars::Triggerbot::fov, 1.0f, 200.0f, "%.0fpx");
                    Menu::SliderFloat("Fire Delay", &Vars::Triggerbot::delay, 0.0f, 300.0f, "%.0fms");
                    Menu::SliderFloat("Hitbox Scale", &Vars::Triggerbot::hitbox_scale, 0.1f, 10.0f, "%.1f");
                    Menu::SliderFloat("Fire Interval", &Vars::Triggerbot::interval, 10.0f, 1000.0f, "%.0fms");
                    ImGui::Text("Target Hitboxes");
                    Menu::CheckBox("Head", &Vars::Triggerbot::hitboxes[0]);
                    Menu::CheckBox("Torso", &Vars::Triggerbot::hitboxes[1]);
                    Menu::CheckBox("Arms", &Vars::Triggerbot::hitboxes[2]);
                    Menu::CheckBox("Legs", &Vars::Triggerbot::hitboxes[3]);
                    Menu::CheckBox("Team Check##tb", &Vars::Triggerbot::teamCheck);
                    Menu::EndChild();
                }
            }
            else if (Vars::selectedTab == 1) // Visuals
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                if (Menu::BeginChild("ESP Visuals", ImVec2(LeftWidth - SidePad, AvailSize.y - SidePad * 2.0f))) {
                    Menu::CheckBox("Enable ESP", &Vars::ESP::enabled);
                    Menu::CheckBox("Team Check", &Vars::ESP::teamCheck);
                    std::vector<const char*> boxModes = { "Off", "2D Box", "Corner Box", "3D Box" };
                    Menu::Combo("Box Mode", &Vars::ESP::boxMode, boxModes);

                    if (Vars::ESP::boxMode != 0) {
                        Menu::CheckBox("Static Box Mode", &Vars::ESP::staticBox);
                        Menu::CheckBox("Gradient Mode", &Vars::ESP::boxGradient);
                        Menu::SliderFloat("Box Thickness", &Vars::ESP::boxThickness, 1.0f, 5.0f, "%.1fpx");
                        ImGui::Text("Box Color");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##boxcol", Vars::ESP::boxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        if (Vars::ESP::boxGradient) {
                            ImGui::Text("Gradient Bottom");
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                            ImGui::ColorEdit4("##boxgradcol", Vars::ESP::boxGradientColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        }
                    }
                    Menu::CheckBox("Head Dot", &Vars::ESP::headDot);
                    if (Vars::ESP::headDot) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##headdotcol", Vars::ESP::headDotColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("View Angles", &Vars::ESP::viewAngle);
                    std::vector<const char*> nameModes = { "Off", "Username", "Display Name" };
                    Menu::Combo("Name Mode", &Vars::ESP::nameMode, nameModes);
                    if (Vars::ESP::nameMode != 0) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##namecol", Vars::ESP::nameColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Draw Distance", &Vars::ESP::distance);
                    if (Vars::ESP::distance) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##distcol", Vars::ESP::distanceColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Gradient Text", &Vars::ESP::textGradient);
                    if (Vars::ESP::textGradient) {
                        ImGui::Text("Text Gradient Bottom");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##textgradcol", Vars::ESP::textGradientColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Draw Health Bar", &Vars::ESP::healthBar);
                    Menu::CheckBox("Draw Health Text", &Vars::ESP::healthText);
                    Menu::CheckBox("Offscreen Indicators", &Vars::ESP::offscreen);
                    Menu::EndChild();
                }

                ImGui::SameLine(0.0f, Spacing);
                if (Menu::BeginChild("Extra Indicators", ImVec2(RightWidth - SidePad, AvailSize.y - SidePad * 2.0f))) {
                    Menu::CheckBox("Snaplines (Tracers)", &Vars::ESP::tracers);
                    if (Vars::ESP::tracers) {
                        std::vector<const char*> tracerOrigins = { "Top", "Center", "Bottom", "Mouse" };
                        Menu::Combo("Tracer Origin", &Vars::ESP::tracerOrigin, tracerOrigins);
                        ImGui::Text("Tracer Color");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##tracercol", Vars::ESP::tracerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Skeleton", &Vars::ESP::skeleton);
                    if (Vars::ESP::skeleton) {
                        Menu::SliderFloat("Skel Thickness", &Vars::ESP::skeletonThickness, 1.0f, 5.0f, "%.1fpx");
                        ImGui::Text("Skeleton Color");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##skelcol", Vars::ESP::skeletonColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Filled Box", &Vars::ESP::filledBox);
                    if (Vars::ESP::filledBox) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##fillcol", Vars::ESP::filledColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    Menu::CheckBox("Custom Crosshair", &Vars::ESP::crosshair);
                    if (Vars::ESP::crosshair) {
                        Menu::SliderFloat("Size", &Vars::ESP::crosshairSize, 1.0f, 30.0f, "%.0f");
                        Menu::SliderFloat("Thickness", &Vars::ESP::crosshairThickness, 1.0f, 10.0f, "%.0f");
                        Menu::SliderFloat("Gap", &Vars::ESP::crosshairGap, 0.0f, 20.0f, "%.0f");
                        Menu::CheckBox("Dot", &Vars::ESP::crosshairDot);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit4("##crosscol", Vars::ESP::crosshairColor);
                    }
                    Menu::EndChild();
                }
            }
            else if (Vars::selectedTab == 2) // World
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                float fullH = AvailSize.y - SidePad * 2.0f;

                if (Menu::BeginChild("Skybox", ImVec2(LeftWidth - SidePad, fullH))) {
                    Menu::CheckBox("Enable Skybox", &Vars::World::skybox);
                    if (Vars::World::skybox) {
                        std::vector<const char*> skyboxes = {
                            "Purple Galaxy", "Evening Glow", "Sunny Day", "Minecraft", "Overcast",
                            "Anime Sky", "Classic Roblox", "Tropical Sunset", "Custom HDRI",
                            "Deep Galaxy", "Cloudy Legacy"
                        };
                        Menu::Combo("Skybox Theme", &Vars::World::skyboxType, skyboxes);
                        Menu::CheckBox("Rotate Skybox", &Vars::World::rotateSkybox);
                        if (Vars::World::rotateSkybox) {
                            Menu::SliderFloat("Rotation Speed", &Vars::World::rotateSpeed, 0.1f, 5.0f, "%.1f");
                        }
                    }
                    Menu::EndChild();
                }

                ImGui::SameLine(0.0f, Spacing);
                if (Menu::BeginChild("World Lighting", ImVec2(RightWidth - SidePad, fullH))) {
                    Menu::CheckBox("Override Ambience", &Vars::World::ambience);
                    if (Vars::World::ambience) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit3("##ambcol", Vars::World::ambienceColor, ImGuiColorEditFlags_NoLabel);
                    }
                    Menu::CheckBox("Override Fog", &Vars::World::fog);
                    if (Vars::World::fog) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::ColorEdit3("##fogcol", Vars::World::fogColor, ImGuiColorEditFlags_NoLabel);
                        Menu::SliderFloat("Fog Distance", &Vars::World::fogDistance, 0.0f, 5000.0f, "%.0f");
                    }
                    Menu::CheckBox("Override Brightness", &Vars::World::brightness);
                    if (Vars::World::brightness) Menu::SliderFloat("Brightness", &Vars::World::brightnessValue, 0.0f, 10.0f, "%.1f");
                    Menu::CheckBox("Override Exposure", &Vars::World::exposure);
                    if (Vars::World::exposure) Menu::SliderFloat("Exposure", &Vars::World::exposureValue, -5.0f, 5.0f, "%.1f");
                    Menu::CheckBox("Override FOV", &Vars::World::fov);
                    if (Vars::World::fov) Menu::SliderFloat("Field of View", &Vars::World::fovValue, 10.0f, 120.0f, "%.0f");
                    Menu::EndChild();
                }
            }
            else if (Vars::selectedTab == 3) // Player
            {
                float startY = ImGui::GetCursorPosY();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                if (Menu::BeginChild("Movement", ImVec2(LeftWidth - SidePad, HalfHeight))) {
                    Menu::CheckBox("Enable WalkSpeed", &Vars::Local::speedEnabled);
                    Menu::SliderFloat("WalkSpeed Value", &Vars::Local::walkSpeed, 16.0f, 300.0f, "%.0f");
                    Menu::CheckBox("Enable JumpPower", &Vars::Local::jumpEnabled);
                    Menu::SliderFloat("JumpPower Value", &Vars::Local::jumpPower, 50.0f, 300.0f, "%.0f");
                    Menu::CheckBox("Infinite Jump", &Vars::Local::infiniteJumpEnabled);
                    Menu::EndChild();
                }

                float BottomBoxY = HalfHeight - 8;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);
                if (Menu::BeginChild("Desync Settings", ImVec2(LeftWidth - SidePad, BottomBoxY))) {
                    Menu::CheckBox("Enable Desync", &Vars::Local::desyncEnabled);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Desync Keybind", &Vars::Local::desyncKey);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Spoofs network replication and physics state.");
                    std::vector<const char*> desyncTypes = { "Server-sided", "Client-sided" };
                    Menu::Combo("Desync Type", &Vars::Local::desyncType, desyncTypes);
                    if (Vars::Local::desyncType == 0) {
                        std::vector<const char*> serverModes = { "Fling (9e9f)", "Position Jitter" };
                        Menu::Combo("Server Mode", &Vars::Local::serverMode, serverModes);
                    }
                    else {
                        std::vector<const char*> clientModes = { "Angular NaN (Safe)", "Linear NaN (Lag)", "Process Suspend" };
                        Menu::Combo("Client Mode", &Vars::Local::clientMode, clientModes);
                    }
                    Menu::EndChild();
                }

                ImGui::SameLine(0.0f, Spacing);
                ImGui::SetCursorPosY(startY + SidePad);
                if (Menu::BeginChild("Flight & Noclip", ImVec2(RightWidth - SidePad, AvailSize.y - SidePad * 2.0f))) {
                    Menu::CheckBox("Enable Flight", &Vars::Local::flightEnabled);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Flight Keybind", &Vars::Local::flightKey);
                    Menu::SliderFloat("Flight Speed", &Vars::Local::flightSpeed, 10.0f, 200.0f, "%.0f");
                    Menu::CheckBox("Enable Noclip", &Vars::Local::noclipEnabled);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.WindowPadding.x + 12.0f);
                    Menu::KeyBind("Noclip Keybind", &Vars::Local::noclipKey);
                    Menu::EndChild();
                }
            }
            else if (Vars::selectedTab == 4) // Rage
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                if (Menu::BeginChild("Anti-Aim", ImVec2(LeftWidth - SidePad, HalfHeight))) {
                    Menu::CheckBox("Enable Anti-Aim", &Vars::Rage::antiAim);
                    Menu::EndChild();
                }
            }
            else if (Vars::selectedTab == 5) // Settings
            {
                float fullH = AvailSize.y - SidePad * 2.0f;
                float MiscHalf = fullH * 0.5f - SidePad * 0.5f;
                float ConfigBoxH = fullH - MiscHalf - SidePad;

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                if (Menu::BeginChild("Settings", ImVec2(LeftWidth - SidePad, MiscHalf))) {
                    Menu::CheckBox("Stream Proof", &Vars::Misc::streamProof);
                    Menu::CheckBox("Fast Launch", &Vars::Misc::fastLaunch);
                    Menu::CheckBox("Show Explorer UI", &Vars::Explorer::enabled);
                    Menu::CheckBox("Auto-rescan on Game Join", &Vars::Misc::autoRescan);
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    float btnW = (ImGui::GetContentRegionAvail().x - Style.ItemSpacing.x);
                    if (ImGui::Button("Rescan Pointers", ImVec2(btnW, 30))) Vars::Misc::rescan = true;
                    Menu::EndChild();
                }

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

                static char cfgNameBuf[64] = "default";
                static std::vector<std::string> cfgList;
                static bool cfgListDirty = true;

                if (Menu::BeginChild("Config", ImVec2(LeftWidth - SidePad, ConfigBoxH))) {
                    ImGui::Text("Config Name");
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputText("##cfgname", cfgNameBuf, sizeof(cfgNameBuf));

                    float btnW2 = (ImGui::GetContentRegionAvail().x - Style.ItemSpacing.x) * 0.5f;
                    if (ImGui::Button("Save##cfg", ImVec2(btnW2, 0))) {
                        if (Config::Save(cfgNameBuf)) NotificationService::Push("Config Saved successfully!", 2.5f);
                        else NotificationService::Push("Failed to save config.", 2.5f);
                        cfgListDirty = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load##cfg", ImVec2(btnW2, 0))) {
                        if (Config::Load(cfgNameBuf)) NotificationService::Push("Config Loaded successfully!", 2.5f);
                        else NotificationService::Push("Failed to load config.", 2.5f);
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (cfgListDirty) {
                        cfgList = Config::ListConfigs();
                        cfgListDirty = false;
                    }

                    ImGui::Text("Saved Configs:");
                    for (auto& name : cfgList) {
                        bool sel = (name == cfgNameBuf);
                        if (ImGui::Selectable(name.c_str(), sel)) {
                            strncpy_s(cfgNameBuf, name.c_str(), sizeof(cfgNameBuf) - 1);
                        }
                    }
                    Menu::EndChild();
                }
            }

            ImGui::EndGroup();
            ImGui::PopClipRect();
        }

        ImGui::End();
    }

    void EndFrame() {
        ImGui::Render();
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (d3dContext && renderTarget) {
            d3dContext->OMSetRenderTargets(1, &renderTarget, nullptr);
            d3dContext->ClearRenderTargetView(renderTarget, clearColor);
        }

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (swapChain) {
            swapChain->Present(0, 0); // VSync disabled for lowest latency
        }
    }

    void Cleanup() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupD3D11();

        if (windowHandle) {
            DestroyWindow(windowHandle);
            windowHandle = nullptr;
        }
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
    }

    HWND GetWindowHandle() const { return windowHandle; }
};