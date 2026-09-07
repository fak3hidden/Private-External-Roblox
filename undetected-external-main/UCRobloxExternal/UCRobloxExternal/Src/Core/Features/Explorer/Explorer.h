#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include "../../../Render/ImGui/imgui.h"
#include <map>
#include <string>

namespace Explorer {
    inline void RenderNode(RBX::RbxInstance instance) {
        if (instance.Addr == 0) return;
        
        std::string name = instance.GetName();
        std::string class_name = instance.GetClass();
        if (name.empty()) name = "Unnamed";
        if (class_name.empty()) class_name = "???";
        
        std::string nodeText = name + " [" + class_name + "]";
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        
        if (class_name == "Workspace" || class_name == "Players") {
            // These directories are usually populated, but no extra flags needed right now it will dynamically load them
        }

        bool isOpen = ImGui::TreeNodeEx((void*)instance.Addr, flags, "%s", nodeText.c_str());
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0x%llX", instance.Addr);
        }
        
        if (isOpen) {
            auto children = instance.GetChildList();
            for (auto& child : children) {
                RenderNode(child);
            }
            ImGui::TreePop();
        }
    }

    inline void RenderExplorer() {
        if (!Vars::Explorer::enabled) return;
        
        ImGui::Begin("Explorer", &Vars::Explorer::enabled);
        if (Globals::dataModel.Addr != 0) {
            RenderNode(Globals::dataModel);
        } else {
            ImGui::Text("DataModel not loaded...");
        }
        ImGui::End();
    }
}
