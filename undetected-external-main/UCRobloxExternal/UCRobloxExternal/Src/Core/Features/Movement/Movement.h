#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include <windows.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <vector>
#include <utility>

namespace Movement {

    inline void RunInfiniteJump() {
        if (!Vars::Local::infiniteJumpEnabled) return;

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;

        auto hrp = character.FindCharacterPart("HumanoidRootPart");
        if (hrp.Addr == 0) return;

        uintptr_t prim = hrp.GetPrimitivePtr();
        if (prim == 0) return;

        auto humanoid = character.FindChildByClass("Humanoid");
        if (humanoid.Addr == 0) return;

        static bool wasSpaceDown = false;
        bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        
        if (isSpaceDown && !wasSpaceDown) {
            // Absolute bulletproof external Infinite Jump:
            // 1. Teleport the player up instantly to detach from the ground.
            RBX::Vec3 pos = Coms->ReadMemory<RBX::Vec3>(prim + Offsets::Primitive::Position);
            pos.Y += (Vars::Local::jumpPower * 0.05f); // Teleport up proportionally
            Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::Position, pos);

            // 2. Kill all downward momentum
            RBX::Vec3 currentVelocity = Coms->ReadMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            currentVelocity.Y = 0.0f;
            Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, currentVelocity);
            
            // 3. Force state to Jumping (5)
            uintptr_t stateObj = Coms->ReadMemory<uintptr_t>(humanoid.Addr + Offsets::Humanoid::HumanoidState);
            if (stateObj) {
                Coms->WriteMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID, 5); 
            }
            Coms->WriteMemory<bool>(humanoid.Addr + Offsets::Humanoid::Jump, true);
        }
        wasSpaceDown = isSpaceDown;
    }

    // Dual-write WalkSpeed (metixud/RobloxExternalBase method): the controller runs
    // at Walkspeed (0x1D0) but integrity compares it against WalkspeedCheck (0x3BC) —
    // writing only one side kicks. Both are written every frame (see ModifyWalkSpeed).
    inline void RunSpeed() {
        static bool prevEnabled = false;
        static bool checkLogged = false;

        if (!Vars::Local::speedEnabled) {
            if (prevEnabled) {
                auto ch = Globals::localPlayer.GetModelRef();
                if (ch.Addr != 0) {
                    auto hum = ch.FindChildByClass("Humanoid");
                    if (hum.Addr != 0) RBX::ModifyWalkspeed(hum, 16.0f);
                }
                prevEnabled = false;
                checkLogged = false;
            }
            return;
        }

        float target = Vars::Local::walkSpeed;
        if (target < 0.0f) target = 0.0f;
        if (target > 300.0f) target = 300.0f;

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;
        auto humanoid = character.FindChildByClass("Humanoid");
        if (humanoid.Addr == 0) return;

        if (!checkLogged) {
            float chk = Coms->ReadMemory<float>(humanoid.Addr + Offsets::Humanoid::WalkspeedCheck);
            std::cout << "[Speed] WalkspeedCheck reads " << chk << "\n";
            checkLogged = true;
        }

        RBX::ModifyWalkspeed(humanoid, target);
        prevEnabled = true;
    }

    inline void RunNoclip() {
        static bool noclipToggled = false;
        static bool wasKeyDown = false;

        if (Vars::Local::noclipKey != 0) {
            bool isKeyDown = (GetAsyncKeyState(Vars::Local::noclipKey) & 0x8000) != 0;
            if (isKeyDown && !wasKeyDown) {
                Vars::Local::noclipEnabled = !Vars::Local::noclipEnabled;
            }
            wasKeyDown = isKeyDown;
        }

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;

        auto parts = character.GetChildList();
        for (auto& part : parts) {
            if (part.GetClass() != "Part" && part.GetClass() != "MeshPart") continue;

            uintptr_t prim = part.GetPrimitivePtr();
            if (prim == 0) continue;

            uintptr_t flagsAddr = prim + Offsets::Primitive::Flags;
            DWORD flags = Coms->ReadMemory<DWORD>(flagsAddr);

            if (Vars::Local::noclipEnabled) {
                // Remove collision bit (Bit 3)
                if (flags & Offsets::PrimitiveFlags::CanCollide) {
                    Coms->WriteMemory<DWORD>(flagsAddr, flags & ~Offsets::PrimitiveFlags::CanCollide);
                }
            } else {
                // Restore collision bit 
                if (!(flags & Offsets::PrimitiveFlags::CanCollide)) {
                    // Only restore it for things that shouldn't be transparent/nonsolid naturally
                    if (part.GetName() == "HumanoidRootPart") continue; 
                    Coms->WriteMemory<DWORD>(flagsAddr, flags | Offsets::PrimitiveFlags::CanCollide);
                }
            }
        }
    }

    inline void RunDesync() {
        static bool wasKeyDown = false;
        static bool isSuspended = false;
        static bool wasDesyncEnabled = false;
        
        static int lastDesyncType = -1;
        static int lastServerMode = -1;
        static int lastClientMode = -1;
        static RBX::Vec3 lastGoodPosition = {0.0f, 0.0f, 0.0f};

        typedef LONG(NTAPI* pfnNtSuspendProcess)(HANDLE);
        typedef LONG(NTAPI* pfnNtResumeProcess)(HANDLE);
        static auto NtSuspendProcess = (pfnNtSuspendProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess");
        static auto NtResumeProcess = (pfnNtResumeProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtResumeProcess");

        if (Vars::Local::desyncKey != 0) {
            bool isKeyDown = (GetAsyncKeyState(Vars::Local::desyncKey) & 0x8000) != 0;
            if (isKeyDown && !wasKeyDown) {
                Vars::Local::desyncEnabled = !Vars::Local::desyncEnabled;
            }
            wasKeyDown = isKeyDown;
        }

        // Restore clean state when desync gets turned off or mode is changed
        bool shouldRestore = false;
        if (wasDesyncEnabled && !Vars::Local::desyncEnabled) {
            shouldRestore = true;
        }
        else if (wasDesyncEnabled && (lastDesyncType != Vars::Local::desyncType || 
                                     (lastDesyncType == 0 && lastServerMode != Vars::Local::serverMode) ||
                                     (lastDesyncType == 1 && lastClientMode != Vars::Local::clientMode))) {
            shouldRestore = true;
        }

        if (shouldRestore) {
            auto character = Globals::localPlayer.GetModelRef();
            if (character.Addr != 0) {
                auto hrp = character.FindCharacterPart("HumanoidRootPart");
                if (hrp.Addr != 0) {
                    uintptr_t prim = hrp.GetPrimitivePtr();
                    if (prim != 0) {
                        int typeToRestore = (lastDesyncType != -1) ? lastDesyncType : Vars::Local::desyncType;
                        int serverModeToRestore = (lastServerMode != -1) ? lastServerMode : Vars::Local::serverMode;
                        int clientModeToRestore = (lastClientMode != -1) ? lastClientMode : Vars::Local::clientMode;

                        if (typeToRestore == 0) { // Server-sided
                            if (serverModeToRestore == 0) { // Fling (9e9f)
                                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, {0.0f, 0.0f, 0.0f});
                            }
                            else if (serverModeToRestore == 1) { // Position Jitter
                                if (lastGoodPosition.X != 0.0f || lastGoodPosition.Y != 0.0f || lastGoodPosition.Z != 0.0f) {
                                    Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::Position, lastGoodPosition);
                                }
                            }
                        }
                        else if (typeToRestore == 1) { // Client-sided
                            if (clientModeToRestore == 0) { // Angular NaN
                                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, {0.0f, 0.0f, 0.0f});
                            }
                            else if (clientModeToRestore == 1) { // Linear NaN
                                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, {0.0f, 0.0f, 0.0f});
                            }
                        }
                    }
                }
            }
        }

        if (!Vars::Local::desyncEnabled || Vars::Local::desyncType != 1 || Vars::Local::clientMode != 2) {
            if (isSuspended && NtResumeProcess) {
                NtResumeProcess(Coms->GetHandle());
                isSuspended = false;
            }
        }

        wasDesyncEnabled = Vars::Local::desyncEnabled;
        if (Vars::Local::desyncEnabled) {
            lastDesyncType = Vars::Local::desyncType;
            lastServerMode = Vars::Local::serverMode;
            lastClientMode = Vars::Local::clientMode;
        } else {
            lastDesyncType = -1;
            lastServerMode = -1;
            lastClientMode = -1;
        }

        if (!Vars::Local::desyncEnabled) return;
        
        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;

        auto hrp = character.FindCharacterPart("HumanoidRootPart");
        if (hrp.Addr == 0) return;
        
        uintptr_t prim = hrp.GetPrimitivePtr();
        if (prim == 0) return;
        
        if (Vars::Local::desyncType == 0) { // Server-sided
            if (Vars::Local::serverMode == 0) { // Fling (9e9f)
                RBX::Vec3 flingVelocity = {9e9f, 9e9f, 9e9f};
                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, flingVelocity);
            }
            else if (Vars::Local::serverMode == 1) { // Position Jitter
                static int tickCount = 0;
                tickCount++;
                
                RBX::Vec3 realPos = Coms->ReadMemory<RBX::Vec3>(prim + Offsets::Primitive::Position);
                if (tickCount % 2 == 0) {
                    lastGoodPosition = realPos;
                    RBX::Vec3 fakePos = realPos;
                    fakePos.Y -= 1000.0f;
                    Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::Position, fakePos);
                } else {
                    Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::Position, realPos);
                }
            }
        }
        else if (Vars::Local::desyncType == 1) { // Client-sided
            if (Vars::Local::clientMode == 0) { // Angular NaN
                float nan = std::numeric_limits<float>::quiet_NaN();
                RBX::Vec3 frozenAngular = {nan, nan, nan};
                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, frozenAngular);
            }
            else if (Vars::Local::clientMode == 1) { // Linear NaN
                float nan = std::numeric_limits<float>::quiet_NaN();
                RBX::Vec3 frozenLinear = {nan, nan, nan};
                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, frozenLinear);
            }
            else if (Vars::Local::clientMode == 2) { // Process Suspend
                if (!isSuspended && NtSuspendProcess) {
                    NtSuspendProcess(Coms->GetHandle());
                    isSuspended = true;
                }
            }
        }
    }
}
\nauto nclipParent = part.GetParent();\nif (nclipParent.IsValid()) { std::string npc = nclipParent.GetClass(); if (npc == "Accessory" || npc == "Hat") continue; }