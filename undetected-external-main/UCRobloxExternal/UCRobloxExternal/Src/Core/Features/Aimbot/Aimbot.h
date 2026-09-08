#pragma once
#include "../../../Game/W2S/W2S.h"
#include "../../../Core/Cache/Cache.h"
#include "../../../Core/Vars/Vars.h"
#include "../../../Game/SDK/SDK.h"
#include <windows.h>
#include <cmath>
#include <random>
#include <algorithm>

namespace Aimbot {

    inline uintptr_t lockedPlayerAddr = 0;

    inline void MoveMouse(float x, float y) 
    {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = static_cast<LONG>(x);
        input.mi.dy = static_cast<LONG>(y);
        SendInput(1, &input, sizeof(INPUT));
    }



    inline float GetDistance2D(RBX::Vec2 a, RBX::Vec2 b) {
        float dx = a.X - b.X;
        float dy = a.Y - b.Y;
        return sqrtf(dx * dx + dy * dy);
    }


    inline RBX::Mat3 LookAt(RBX::Vec3 camera_pos, RBX::Vec3 target_pos) {
        RBX::Vec3 forward = { target_pos.X - camera_pos.X, target_pos.Y - camera_pos.Y, target_pos.Z - camera_pos.Z };
        float fLen = sqrtf(forward.X * forward.X + forward.Y * forward.Y + forward.Z * forward.Z);
        if (fLen > 0.0001f) {
            forward.X /= fLen; forward.Y /= fLen; forward.Z /= fLen;
        } else {
            forward = { 0, 0, -1 };
        }

        RBX::Vec3 up = { 0, 1, 0 };
        RBX::Vec3 right = {
            forward.Y * up.Z - forward.Z * up.Y,
            forward.Z * up.X - forward.X * up.Z,
            forward.X * up.Y - forward.Y * up.X
        };
        float rLen = sqrtf(right.X * right.X + right.Y * right.Y + right.Z * right.Z);
        if (rLen > 0.0001f) {
            right.X /= rLen; right.Y /= rLen; right.Z /= rLen;
        } else {
            right = { 1, 0, 0 };
        }

        RBX::Vec3 newUp = {
            right.Y * forward.Z - right.Z * forward.Y,
            right.Z * forward.X - right.X * forward.Z,
            right.X * forward.Y - right.Y * forward.X
        };

        RBX::Mat3 result;
        result.data[0] = right.X; result.data[1] = newUp.X; result.data[2] = -forward.X;
        result.data[3] = right.Y; result.data[4] = newUp.Y; result.data[5] = -forward.Y;
        result.data[6] = right.Z; result.data[7] = newUp.Z; result.data[8] = -forward.Z;
        return result;
    }

    // boneId 6 = random — picks a random valid part from the set each call
    inline RBX::RbxInstance GetBonePart(RBX::RbxInstance character, int boneId) {
        if (boneId == 6) {
            // Random: try each part in a shuffled order and return first valid
            static thread_local std::mt19937 rng(std::random_device{}());
            const char* candidates[] = {
                "Head", "HumanoidRootPart", "Root",
                "LeftLowerArm",  "Left Arm",
                "RightLowerArm", "Right Arm",
                "LeftLowerLeg",  "Left Leg",
                "RightLowerLeg", "Right Leg",
                "UpperTorso",    "Torso"
            };
            std::vector<const char*> pool(std::begin(candidates), std::end(candidates));
            std::shuffle(pool.begin(), pool.end(), rng);
            for (auto* name : pool) {
                auto part = character.FindCharacterPart(name);
                if (part.Addr != 0) return part;
            }
            return character.FindCharacterPart("Head");
        }
        if (boneId == 0) return character.FindCharacterPart("Head");
        if (boneId == 1) {
            auto part = character.FindCharacterPart("HumanoidRootPart");
            return part.Addr != 0 ? part : character.FindCharacterPart("Root");
        }
        if (boneId == 2) {
            auto part = character.FindCharacterPart("LeftLowerArm");
            return part.Addr != 0 ? part : character.FindCharacterPart("Left Arm");
        }
        if (boneId == 3) {
            auto part = character.FindCharacterPart("RightLowerArm");
            return part.Addr != 0 ? part : character.FindCharacterPart("Right Arm");
        }
        if (boneId == 4) {
            auto part = character.FindCharacterPart("LeftLowerLeg");
            return part.Addr != 0 ? part : character.FindCharacterPart("Left Leg");
        }
        if (boneId == 5) {
            auto part = character.FindCharacterPart("RightLowerLeg");
            return part.Addr != 0 ? part : character.FindCharacterPart("Right Leg");
        }
        return character.FindCharacterPart("Head");
    }

    inline RBX::Vec3 GetPredictedPos(RBX::RbxInstance targetPart, RBX::Vec3 pos) {
        if (!Vars::Aimbot::prediction) return pos;
        uintptr_t prim = targetPart.GetPrimitivePtr();
        if (prim != 0) {
            RBX::Vec3 vel = Coms->ReadMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            float px = Vars::Aimbot::predX != 0 ? (2.1f - Vars::Aimbot::predX) : 0.0f;
            float py = Vars::Aimbot::predY != 0 ? (2.1f - Vars::Aimbot::predY) : 0.0f;
            pos.X += Vars::Aimbot::predX != 0 ? vel.X * px : 0.0f;
            pos.Y += Vars::Aimbot::predY != 0 ? vel.Y * py : 0.0f;
            pos.Z += Vars::Aimbot::predX != 0 ? vel.Z * px : 0.0f;
        }
        return pos;
    }
    
    inline RBX::Vec3 GetSilentPredictedPos(RBX::RbxInstance targetPart, RBX::Vec3 pos) {
        if (!Vars::Aimbot::silentPrediction) return pos;
        uintptr_t prim = targetPart.GetPrimitivePtr();
        if (prim != 0) {
            RBX::Vec3 vel = Coms->ReadMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            float px = Vars::Aimbot::predX != 0 ? (2.1f - Vars::Aimbot::predX) : 0.0f;
            float py = Vars::Aimbot::predY != 0 ? (2.1f - Vars::Aimbot::predY) : 0.0f;
            pos.X += Vars::Aimbot::predX != 0 ? vel.X * px : 0.0f;
            pos.Y += Vars::Aimbot::predY != 0 ? vel.Y * py : 0.0f;
            pos.Z += Vars::Aimbot::predX != 0 ? vel.Z * px : 0.0f;
        }
        return pos;
    }

    inline bool IsTeammate(uintptr_t plrAddr) {
        uintptr_t localTeam  = Coms->ReadMemory<uintptr_t>(Globals::localPlayer.Addr + Offsets::Player::Team);
        uintptr_t targetTeam = Coms->ReadMemory<uintptr_t>(plrAddr + Offsets::Player::Team);
        if (localTeam == 0 || targetTeam == 0) return false;
        int localColor  = Coms->ReadMemory<int>(localTeam  + Offsets::Team::BrickColor);
        int targetColor = Coms->ReadMemory<int>(targetTeam + Offsets::Team::BrickColor);
        return (localColor == targetColor);
    }

    inline bool IsInvisible(RBX::RbxInstance character) {
        const char* parts[] = {
            "Head", "Torso", "HumanoidRootPart", "Root",
            "Left Arm", "Right Arm", "Left Leg", "Right Leg",
            "UpperTorso", "LowerTorso",
            "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightUpperArm", "RightLowerArm", "RightHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        for (const char* p : parts) {
            auto part = character.FindCharacterPart(p);
            if (part.Addr != 0) {
                float t = Coms->ReadMemory<float>(part.Addr + Offsets::BasePart::Transparency);
                if (t < 1.0f) return false;
            }
        }
        return true;
    }
    
    // --- Silent Aim (MouseService write) ---
    // Finds MouseService -> InputObject, then writes the target screen pos to MousePosition.
    // MouseService is resolved from the DataModel (static pointers change every build).
    inline void WriteSilentMousePos(RBX::Vec2 screenPos) {
        uintptr_t msBase = 0;
        if (Globals::dataModel.Addr != 0)
            msBase = Globals::dataModel.FindChildByClass("MouseService").Addr;
        if (msBase == 0) return;
        // InputObject is a shared_ptr; the raw ptr sits 8 bytes in. Two known layouts: try both.
        uintptr_t inputObj = Coms->ReadMemory<uintptr_t>(msBase + Offsets::MouseService::InputObject + 0x8);
        if (inputObj == 0 || inputObj == 0xFFFFFFFFFFFFFFFF)
            inputObj = Coms->ReadMemory<uintptr_t>(msBase + Offsets::MouseService::InputObject2 + 0x8);
        if (inputObj == 0 || inputObj == 0xFFFFFFFFFFFFFFFF) return;
        RBX::Vec2 pos = { screenPos.X, screenPos.Y };
        Coms->WriteMemory<RBX::Vec2>(inputObj + Offsets::MouseService::MousePosition, pos);
    }

    inline void RunAimbot(const RBX::Mat4& viewMatrix) {
        bool silentKeyPressed = false;
        if (Vars::Aimbot::silentAimbotKey > 0 && Vars::Aimbot::silentAimbotKey < 256)
            silentKeyPressed = (GetAsyncKeyState(Vars::Aimbot::silentAimbotKey) & 0x8000);

        POINT mousePos;
        GetCursorPos(&mousePos);
        RBX::Vec2 aimCenter = { static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) };
        float screenW = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
        float screenH = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));

        // --- Silent Aim ---
        if (Vars::Aimbot::silentAim && silentKeyPressed) {
            float closestDist = 999999.0f;
            RBX::Vec3 bestTargetWorld = { 0, 0, 0 };
            RBX::Vec2 bestTargetScreen = { 0, 0 };
            bool foundSilentTarget = false;

            for (auto& plr : PlayerCache::players) {
                if (!plr.isValid) continue;
                auto character = RBX::RbxInstance(plr.characterAddr);

                if (Vars::Aimbot::downedCheck && plr.health <= 4) continue;
                if (Vars::Aimbot::teamCheck && plr.isTeammate) continue;
                if (Vars::Aimbot::invincibleCheck && character.IsInvincible()) continue;
                if (Vars::Aimbot::invisCheck && IsInvisible(character)) continue;

                int targetBone = Vars::Aimbot::randomTarget ? 6 : Vars::Aimbot::aimTarget;
                RBX::RbxInstance targetPart = GetBonePart(character, targetBone);
                if (targetPart.Addr == 0) continue;

                RBX::Vec3 targetPos = targetPart.GetPos();
                targetPos = GetSilentPredictedPos(targetPart, targetPos);
                RBX::Vec2 screenPos = W2S::WorldToScreen(targetPos, viewMatrix);

                if (screenPos.X == 0 && screenPos.Y == 0) continue;
                if (screenPos.X < 0 || screenPos.Y < 0 || screenPos.X > screenW || screenPos.Y > screenH) continue;

                float dist = GetDistance2D(aimCenter, screenPos);
                if (dist < Vars::Aimbot::silentFOV && dist < closestDist) {
                    closestDist = dist;
                    bestTargetWorld = targetPos;
                    bestTargetScreen = screenPos;
                    foundSilentTarget = true;
                }
            }

            if (foundSilentTarget) {
                WriteSilentMousePos(bestTargetScreen);
                if (Globals::camera.Addr != 0) {
                    RBX::Vec3 cameraPos = Globals::camera.GetCameraPosition();
                    RBX::Mat3 originalRot = Globals::camera.GetCameraRotation();
                    RBX::Mat3 targetRot = LookAt(cameraPos, bestTargetWorld);
                    
                    Globals::camera.SetCameraRotation(targetRot);
                    
                    // Busy-wait briefly so engine updates look-vector during physics frame tick
                    LARGE_INTEGER frequency, start, current;
                    QueryPerformanceFrequency(&frequency);
                    QueryPerformanceCounter(&start);
                    double elapsed = 0.0;
                    while (elapsed < 0.0008) { // 800 microseconds
                        QueryPerformanceCounter(&current);
                        elapsed = static_cast<double>(current.QuadPart - start.QuadPart) / frequency.QuadPart;
                    }
                    
                    Globals::camera.SetCameraRotation(originalRot);
                }
            }
        }

        // --- Standard Aimbot Logic ---
        if (!Vars::Aimbot::enabled) return;

        bool keyPressed = false;
        if (Vars::Aimbot::aimbotKey > 0 && Vars::Aimbot::aimbotKey < 256) {
            keyPressed = (GetAsyncKeyState(Vars::Aimbot::aimbotKey) & 0x8000);
        }

        if (!keyPressed) 
        {
            lockedPlayerAddr = 0;
            return;
        }

        if (lockedPlayerAddr == 0) {
            float closestDist = 999999.0f;
            uintptr_t closestPlayerAddr = 0;

            for(auto& plr : PlayerCache::players){
                if(!plr.isValid) continue;
                auto character = RBX::RbxInstance(plr.characterAddr);
                
                if(Vars::Aimbot::downedCheck && plr.health <= 4) continue;
                if(Vars::Aimbot::teamCheck && plr.isTeammate) continue;
                if(Vars::Aimbot::invincibleCheck && character.IsInvincible()) continue;
                if(Vars::Aimbot::invisCheck && IsInvisible(character)) continue;

                RBX::RbxInstance targetPart = GetBonePart(character, Vars::Aimbot::aimTarget);

                if (targetPart.Addr == 0) continue;

                RBX::Vec3 targetPos = targetPart.GetPos();
                targetPos = GetPredictedPos(targetPart, targetPos);
                RBX::Vec2 screenPos = W2S::WorldToScreen(targetPos, viewMatrix);

                if (screenPos.X == 0 && screenPos.Y == 0) continue;
                if (screenPos.X < 0 || screenPos.Y < 0 || screenPos.X > screenW || screenPos.Y > screenH) continue;

                float dist = GetDistance2D(aimCenter, screenPos);

                if (dist < Vars::Aimbot::fovRadius && dist < closestDist) {
                    closestDist = dist;
                    closestPlayerAddr = plr.playerAddr;
                }
            }

            if (closestPlayerAddr != 0) {
                lockedPlayerAddr = closestPlayerAddr;
            }
        }

        if (lockedPlayerAddr != 0) {
            bool foundLockedPlayer = false;
            RBX::Vec2 targetScreenPos = { 0, 0 };
            RBX::Vec3 targetWorldPos = { 0, 0, 0 };

            for(auto& plr : PlayerCache::players){
                if(!plr.isValid) continue;
                if(plr.playerAddr != lockedPlayerAddr) continue;
                
                auto character = RBX::RbxInstance(plr.characterAddr);

                if(Vars::Aimbot::downedCheck && plr.health <= 4) break;
                if(Vars::Aimbot::teamCheck && plr.isTeammate) break;
                if(character.Addr == 0) break;
                if(Vars::Aimbot::invincibleCheck && character.IsInvincible()) break;
                if(Vars::Aimbot::invisCheck && character.IsTransparent()) break;

                RBX::RbxInstance targetPart = GetBonePart(character, Vars::Aimbot::aimTarget);

                if (targetPart.Addr == 0) break;

                targetWorldPos = targetPart.GetPos();
                targetWorldPos = GetPredictedPos(targetPart, targetWorldPos);
                RBX::Vec2 screenPos = W2S::WorldToScreen(targetWorldPos, viewMatrix);

                if (screenPos.X == 0 && screenPos.Y == 0) break;
                if (screenPos.X < 0 || screenPos.Y < 0 || screenPos.X > screenW || screenPos.Y > screenH) break;

                targetScreenPos = screenPos;
                foundLockedPlayer = true;
                break;
            }

            if (!foundLockedPlayer) {
                lockedPlayerAddr = 0;
                return;
            }

            if (Vars::Aimbot::aimMethod == 1) {
                RBX::Vec3 cameraPos = Globals::camera.GetCameraPosition();
                RBX::Mat3 targetRot = LookAt(cameraPos, targetWorldPos);
                
                if (Vars::Aimbot::smoothing > 1.0f) {
                    RBX::Mat3 cameraRot = Globals::camera.GetCameraRotation();
                    float t = Vars::Aimbot::smoothing * 10.0f;
                    float k = 1.0f / t;
                    RBX::Mat3 result{};
                    for (int i = 0; i < 9; ++i) {
                        result.data[i] = cameraRot.data[i] + (targetRot.data[i] - cameraRot.data[i]) * k;
                    }
                    Globals::camera.SetCameraRotation(result);
                } else {
                    Globals::camera.SetCameraRotation(targetRot);
                }
            } else {
                float dx = targetScreenPos.X - mousePos.x;
                float dy = targetScreenPos.Y - mousePos.y;
                float smoothFactor = 1.0f / Vars::Aimbot::smoothing;
                MoveMouse(dx * smoothFactor, dy * smoothFactor);
            }
        }
    }

    inline void RunTriggerbot(const RBX::Mat4& viewMatrix) {
        if (!Vars::Triggerbot::enabled) return;

        bool keyHeld = Vars::Triggerbot::key <= 0 || (GetAsyncKeyState(Vars::Triggerbot::key) & 0x8000);
        if (!keyHeld) return;

        static std::chrono::steady_clock::time_point last_shot_time;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_shot_time).count() < Vars::Triggerbot::interval)
            return;

        POINT cur;
        GetCursorPos(&cur);
        RBX::Vec2 center = { static_cast<float>(cur.x), static_cast<float>(cur.y) };

        bool found_target = false;
        
        for (auto& plr : PlayerCache::players) {
            if (!plr.isValid) continue;
            if (Vars::Triggerbot::teamCheck && plr.isTeammate) continue;
            if (plr.health <= 0) continue;
            
            auto character = RBX::RbxInstance(plr.characterAddr);
            if (character.Addr == 0) continue;
            
            if (Vars::Aimbot::invincibleCheck && character.IsInvincible()) continue;
            if (Vars::Aimbot::invisCheck && character.IsTransparent()) continue;

            std::vector<std::string> parts_to_check;

            if (Vars::Triggerbot::hitboxes[0]) parts_to_check.push_back("Head");
            if (Vars::Triggerbot::hitboxes[1]) {
                parts_to_check.push_back("UpperTorso");
                parts_to_check.push_back("LowerTorso");
                parts_to_check.push_back("Torso");
            }
            if (Vars::Triggerbot::hitboxes[2]) {
                parts_to_check.push_back("LeftUpperArm");
                parts_to_check.push_back("RightUpperArm");
                parts_to_check.push_back("LeftLowerArm");
                parts_to_check.push_back("RightLowerArm");
                parts_to_check.push_back("Left Arm");
                parts_to_check.push_back("Right Arm");
            }
            if (Vars::Triggerbot::hitboxes[3]) {
                parts_to_check.push_back("LeftUpperLeg");
                parts_to_check.push_back("RightUpperLeg");
                parts_to_check.push_back("LeftLowerLeg");
                parts_to_check.push_back("RightLowerLeg");
                parts_to_check.push_back("Left Leg");
                parts_to_check.push_back("Right Leg");
            }

            for (const auto& part_name : parts_to_check) {
                auto part = character.FindCharacterPart(part_name);
                if (part.Addr != 0) {
                    RBX::Vec3 pos = part.GetPos();
                    RBX::Vec2 screen_pos = W2S::WorldToScreen(pos, viewMatrix);

                    if (screen_pos.X == 0 && screen_pos.Y == 0) continue;

                    float dist = static_cast<float>(std::sqrt(std::pow(screen_pos.X - center.X, 2) + std::pow(screen_pos.Y - center.Y, 2)));

                    if (dist < (Vars::Triggerbot::hitbox_scale * 5.0f)) {
                        found_target = true;
                        break;
                    }
                }
            }
            if (found_target) break;
        }

        if (found_target) {
            if (Vars::Triggerbot::delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)Vars::Triggerbot::delay));
            }

            INPUT dn = {};
            dn.type = INPUT_MOUSE;
            dn.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &dn, sizeof(INPUT));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            INPUT up = {};
            up.type = INPUT_MOUSE;
            up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &up, sizeof(INPUT));

            last_shot_time = std::chrono::steady_clock::now();
        }
    }
}

