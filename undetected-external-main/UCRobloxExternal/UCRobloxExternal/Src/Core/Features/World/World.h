#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include <windows.h>

namespace World {
    inline void RunWorld() {
        if (Globals::dataModel.Addr == 0) return;

        auto lighting = Globals::dataModel.FindChildByClass("Lighting");
        if (lighting.Addr != 0) {
            bool needsInvalidate = false;

            if (Vars::World::skybox) {
                auto sky = lighting.FindChildByClass("Sky");
                if (sky.Addr != 0) {
                    static int lastSkybox = -1;
                    static uintptr_t lastSkyAddr = 0;
                    if (Vars::World::skyboxType != lastSkybox || sky.Addr != lastSkyAddr) {
                        lastSkybox = Vars::World::skyboxType;
                        lastSkyAddr = sky.Addr;
                        
                        if (Vars::World::skyboxType == 0) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://12635309703");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://12635311686");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://12635312870");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://12635313718");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://12635315817");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://12635316856");
                        }
                        else if (Vars::World::skyboxType == 1) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://12064107");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://12064152");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://12064121");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://12063984");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://12064115");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://12064131");
                        }
                        else if (Vars::World::skyboxType == 2) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://271042516");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://271077243");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://271042556");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://271042310");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://271042467");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://271077958");
                        }
                        else if (Vars::World::skyboxType == 3) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://1876545003");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://1876544331");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://1876542941");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://1876543392");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://1876543764");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://1876544642");
                        }
                        else if (Vars::World::skyboxType == 4) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://116758234");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://116758314");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://116758367");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://116758446");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://116758478");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://116758496");
                        }
                        else if (Vars::World::skyboxType == 5) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://1233158420");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://1233158838");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://1233157105");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://1233157640");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://1233157995");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://1233159158");
                        }
                        else if (Vars::World::skyboxType == 6) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://1327358");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://1327359");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://1327355");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://1327357");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://1327356");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://1327360");
                        }
                        else if (Vars::World::skyboxType == 7) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://570555736");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://570555964");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://570555800");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://570555840");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://570555882");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://570555929");
                        }
                        else if (Vars::World::skyboxType == 8) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://95020137072033");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://92862258103959");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://107665368823185");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://126542804346203");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://103716549795832");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://131036626982613");
                        }
                        else if (Vars::World::skyboxType == 9) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://169210090");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://169210108");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://169210121");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://169210133");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://169210143");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://169210149");
                        }
                        else if (Vars::World::skyboxType == 10) {
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxBk, "rbxassetid://47974894");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxDn, "rbxassetid://47974690");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxFt, "rbxassetid://47974821");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxLf, "rbxassetid://47974776");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxRt, "rbxassetid://47974859");
                            Coms->WriteGameString(sky.Addr + Offsets::Sky::SkyboxUp, "rbxassetid://47974909");
                        }
                        needsInvalidate = true;
                    }

                    if (Vars::World::rotateSkybox) {
                        static float rotY = 0.0f;
                        rotY += Vars::World::rotateSpeed;
                        if (rotY > 360.0f) rotY -= 360.0f;
                        Coms->WriteMemory<RBX::Vec3>(sky.Addr + Offsets::Sky::SkyboxOrientation, {0.0f, rotY, 0.0f});
                    }

                    needsInvalidate = true;
                }
            }

            if (Vars::World::ambience) {
                RBX::Vec3 amb = {Vars::World::ambienceColor[0], Vars::World::ambienceColor[1], Vars::World::ambienceColor[2]};
                Coms->WriteMemory<RBX::Vec3>(lighting.Addr + Offsets::Lighting::Ambient, amb);
                Coms->WriteMemory<RBX::Vec3>(lighting.Addr + Offsets::Lighting::OutdoorAmbient, amb);
                needsInvalidate = true;
            }

            if (Vars::World::fog) {
                RBX::Vec3 fColor = {Vars::World::fogColor[0], Vars::World::fogColor[1], Vars::World::fogColor[2]};
                Coms->WriteMemory<RBX::Vec3>(lighting.Addr + Offsets::Lighting::FogColor, fColor);
                Coms->WriteMemory<float>(lighting.Addr + Offsets::Lighting::FogEnd, Vars::World::fogDistance);
                needsInvalidate = true;
            }

            if (Vars::World::brightness) {
                Coms->WriteMemory<float>(lighting.Addr + Offsets::Lighting::Brightness, Vars::World::brightnessValue);
                needsInvalidate = true;
            }

            if (Vars::World::exposure) {
                Coms->WriteMemory<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation, Vars::World::exposureValue);
                needsInvalidate = true;
            }
            
            if (needsInvalidate && Globals::renderEngine.Addr != 0 && Offsets::VisualEngine::RenderViewOk) {
                uintptr_t renderView = Coms->ReadMemory<uintptr_t>(Globals::renderEngine.Addr + Offsets::VisualEngine::RenderView);
                if (renderView != 0) {
                    Coms->WriteMemory<uint8_t>(renderView + Offsets::RenderView::LightingValid, 0);
                    Coms->WriteMemory<uint8_t>(renderView + Offsets::RenderView::SkyValid, 0);
                }
            }
        }

        if (Vars::World::fov && Globals::camera.Addr != 0) {
            Coms->WriteMemory<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, Vars::World::fovValue);
        }
    }
}
