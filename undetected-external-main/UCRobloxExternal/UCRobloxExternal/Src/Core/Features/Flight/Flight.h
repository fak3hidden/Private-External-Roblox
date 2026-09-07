#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include <windows.h>

namespace Flight {
    inline void RunFlight() {
        static bool isFlying = false;
        static bool wasKeyPressed = false;

        if (Vars::Local::flightEnabled) {
            if (Vars::Local::flightKey > 0 && Vars::Local::flightKey < 256) {
                bool isKeyDown = !Vars::menuOpen && ((GetAsyncKeyState(Vars::Local::flightKey) & 0x8000) != 0);
                if (isKeyDown && !wasKeyPressed) {
                    isFlying = !isFlying;
                    if (!isFlying) {
                        auto character = Globals::localPlayer.GetModelRef();
                        if (character.Addr != 0) {
                            auto humanoid = character.FindChildByClass("Humanoid");
                            if (humanoid.Addr != 0) {
                                bool currentPlatformStand = Coms->ReadMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand);
                                if (currentPlatformStand) {
                                    Coms->WriteMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand, false);
                                }
                                uintptr_t stateObj = Coms->ReadMemory<uintptr_t>(humanoid.Addr + Offsets::Humanoid::HumanoidState);
                                if (stateObj) {
                                    BYTE currentState = Coms->ReadMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID);
                                    if (currentState == 16 || currentState == 14) {
                                        Coms->WriteMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID, 8);
                                    }
                                }
                                auto hrp = character.FindCharacterPart("HumanoidRootPart");
                                if (hrp.Addr != 0) {
                                    uintptr_t prim = hrp.GetPrimitivePtr();
                                    if (prim != 0) {
                                        Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, {0, 0, 0});
                                    }
                                }
                            }
                        }
                    }
                }
                wasKeyPressed = isKeyDown;
            }
        } else {
            if (isFlying) {
                auto character = Globals::localPlayer.GetModelRef();
                if (character.Addr != 0) {
                    auto humanoid = character.FindChildByClass("Humanoid");
                    if (humanoid.Addr != 0) {
                        bool currentPlatformStand = Coms->ReadMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand);
                        if (currentPlatformStand) {
                            Coms->WriteMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand, false);
                        }
                        uintptr_t stateObj = Coms->ReadMemory<uintptr_t>(humanoid.Addr + Offsets::Humanoid::HumanoidState);
                        if (stateObj) {
                            BYTE currentState = Coms->ReadMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID);
                            if (currentState == 16 || currentState == 14) {
                                Coms->WriteMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID, 8);
                            }
                        }
                        auto hrp = character.FindCharacterPart("HumanoidRootPart");
                        if (hrp.Addr != 0) {
                            uintptr_t prim = hrp.GetPrimitivePtr();
                            if (prim != 0) {
                                Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, {0, 0, 0});
                            }
                        }
                    }
                }
            }
            isFlying = false;
        }

        if (!isFlying) return;

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;

        auto hrp = character.FindCharacterPart("HumanoidRootPart");
        if (hrp.Addr == 0) return;

        uintptr_t prim = hrp.GetPrimitivePtr();
        if (prim == 0) return;

        bool airCheck = false;
        RBX::Mat3 rot = Globals::camera.GetCameraRotation();
        
        RBX::Vec3 lookVector = { -rot.data[2], -rot.data[5], -rot.data[8] };
        RBX::Vec3 rightVector = { rot.data[0], rot.data[3], rot.data[6] };
        RBX::Vec3 direction = { 0, 0, 0 };
        
        bool w = !Vars::menuOpen && (GetAsyncKeyState('W') & 0x8000);
        bool s = !Vars::menuOpen && (GetAsyncKeyState('S') & 0x8000);
        bool a = !Vars::menuOpen && (GetAsyncKeyState('A') & 0x8000);
        bool d = !Vars::menuOpen && (GetAsyncKeyState('D') & 0x8000);
        bool space = !Vars::menuOpen && (GetAsyncKeyState(VK_SPACE) & 0x8000);
        bool ctrl = !Vars::menuOpen && (GetAsyncKeyState(VK_LCONTROL) & 0x8000);

        if (w) {
            direction.X += lookVector.X;
            direction.Y += lookVector.Y;
            direction.Z += lookVector.Z;
            airCheck = true;
        }
        if (s) {
            direction.X -= lookVector.X;
            direction.Y -= lookVector.Y;
            direction.Z -= lookVector.Z;
            airCheck = true;
        }
        if (d) {
            direction.X += rightVector.X;
            direction.Y += rightVector.Y;
            direction.Z += rightVector.Z;
            airCheck = true;
        }
        if (a) {
            direction.X -= rightVector.X;
            direction.Y -= rightVector.Y;
            direction.Z -= rightVector.Z;
            airCheck = true;
        }

        if (space) {
            direction.Y += 1.0f;
            airCheck = true;
        }
        if (ctrl) {
            direction.Y -= 1.0f;
            airCheck = true;
        }
        
        float mag = sqrtf(direction.X*direction.X + direction.Y*direction.Y + direction.Z*direction.Z);
        if (mag > 0.001f) {
            direction.X /= mag;
            direction.Y /= mag;
            direction.Z /= mag;
        }

        auto humanoid = character.FindChildByClass("Humanoid");
        if (humanoid.Addr != 0) {
            bool currentPlatformStand = Coms->ReadMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand);
            if (!currentPlatformStand) {
                Coms->WriteMemory<bool>(humanoid.Addr + Offsets::Humanoid::PlatformStand, true);
            }
            uintptr_t stateObj = Coms->ReadMemory<uintptr_t>(humanoid.Addr + Offsets::Humanoid::HumanoidState);
            if (stateObj) {
                BYTE currentState = Coms->ReadMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID);
                if (currentState != 16) {
                    Coms->WriteMemory<BYTE>(stateObj + Offsets::Humanoid::HumanoidStateID, 16);
                }
            }
        }

        if (!airCheck) {
            Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, {0, 0, 0});
        } else {
            RBX::Vec3 vel = {
                direction.X * Vars::Local::flightSpeed,
                direction.Y * Vars::Local::flightSpeed,
                direction.Z * Vars::Local::flightSpeed
            };
            Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
        }
    }
}
