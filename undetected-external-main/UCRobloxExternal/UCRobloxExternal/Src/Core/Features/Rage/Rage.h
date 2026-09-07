#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include <cmath>

namespace Rage {
    inline void RunRage() {
        if (!Vars::Rage::antiAim) return;

        static float angle = 0.0f;
        angle += 0.067f;
        if (angle > 6.283185f) { // 2 pi
            angle -= 6.283185f;
        }

        auto character = Globals::localPlayer.GetModelRef();
        if (character.Addr == 0) return;

        auto humanoid = character.FindChildByClass("Humanoid");
        auto hrp = character.FindCharacterPart("HumanoidRootPart");

        if (humanoid.Addr != 0 && hrp.Addr != 0) {
            Coms->WriteMemory<std::uint8_t>(humanoid.Addr + Offsets::Humanoid::AutoRotate, 0);

            uintptr_t prim = hrp.GetPrimitivePtr();
            if (prim != 0) {
                float c = std::cos(angle);
                float s = std::sin(angle);

                RBX::Mat3 rot_y = {{
                    c, 0.f, -s,
                    0.f, 1.f, 0.f,
                    s, 0.f, c
                }};

                Coms->WriteMemory<RBX::Mat3>(prim + Offsets::Primitive::Rotation, rot_y);
            }
        }
    }
}
