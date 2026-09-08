#pragma once
#include "../../../Game/SDK/SDK.h"
#include "../../Vars/Vars.h"
#include "../../Globals/Globals.h"
#include "../../../Render/ImGui/imgui.h"
#include "../../../Render/ImGui/colors.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <chrono>
#include <cstdio>
#include <cctype>

namespace Explorer {

    struct ExpNode {
        uintptr_t addr = 0;
        std::string name;
        std::string cls;
        std::vector<ExpNode> children;
        bool fetched = false;
    };

    struct FlatHit {
        uintptr_t addr = 0;
        std::string name;
        std::string cls;
    };

    inline ExpNode root;
    inline bool rootInit = false;
    inline uintptr_t selectedAddr = 0;
    inline char searchBuf[128] = {};
    inline std::vector<FlatHit> searchHits;
    inline std::vector<FlatHit> deepHits;
    inline bool deepDone = false;
    inline int deepScanned = 0;
    inline bool autoRefresh = true;
    inline std::chrono::steady_clock::time_point lastRef = std::chrono::steady_clock::now();
    inline std::unordered_set<uintptr_t> openSet;

    // ---------------- helpers ----------------
    inline std::string ToLower(const std::string& s) {
        std::string o = s;
        for (auto& c : o) c = (char)tolower((unsigned char)c);
        return o;
    }

    inline bool MatchI(const std::string& hay, const std::string& needleLower) {
        if (needleLower.empty()) return true;
        return ToLower(hay).find(needleLower) != std::string::npos;
    }

    inline void FetchChildren(ExpNode& n) {
        n.children.clear();
        if (n.addr == 0) { n.fetched = true; return; }
        RBX::RbxInstance inst(n.addr);
        auto kids = inst.GetChildList();
        n.children.reserve(kids.size());
        for (auto& k : kids) {
            ExpNode c;
            c.addr = k.Addr;
            c.name = k.GetName();
            c.cls = k.GetClass();
            if (c.name.empty()) c.name = "Unnamed";
            if (c.cls.empty()) c.cls = "???";
            n.children.push_back(std::move(c));
        }
        n.fetched = true;
    }

    inline void RefreshOpen(ExpNode& n) {
        if (n.addr != 0 && n.fetched && (n.addr == root.addr || openSet.count(n.addr))) {
            FetchChildren(n);
            for (auto& c : n.children) RefreshOpen(c);
        }
    }

    inline void CollectMatches(ExpNode& n, const std::string& q, std::vector<FlatHit>& out) {
        if (out.size() >= 300) return;
        for (auto& c : n.children) {
            if (MatchI(c.name, q) || MatchI(c.cls, q)) {
                out.push_back({ c.addr, c.name, c.cls });
                if (out.size() >= 300) return;
            }
            if (c.fetched) CollectMatches(c, q, out);
        }
    }

    inline void DeepScan(uintptr_t addr, const std::string& q, int depth) {
        if (addr == 0 || depth > 16) return;
        if (deepScanned >= 30000 || deepHits.size() >= 500) return;
        RBX::RbxInstance inst(addr);
        auto kids = inst.GetChildList();
        for (auto& k : kids) {
            if (++deepScanned > 30000 || deepHits.size() >= 500) return;
            std::string nm = k.GetName(), cl = k.GetClass();
            if (nm.empty()) nm = "Unnamed";
            if (cl.empty()) cl = "???";
            if (MatchI(nm, q) || MatchI(cl, q))
                deepHits.push_back({ k.Addr, nm, cl });
            DeepScan(k.Addr, q, depth + 1);
        }
    }

    inline std::string BuildPath(uintptr_t addr) {
        std::string path;
        RBX::RbxInstance cur(addr);
        for (int i = 0; i < 24 && cur.Addr != 0; ++i) {
            std::string nm = cur.GetName();
            if (nm.empty()) nm = "???";
            path = path.empty() ? nm : (nm + "." + path);
            if (cur.Addr == Globals::dataModel.Addr) break;
            cur = cur.GetParent();
        }
        return path.empty() ? "<invalid>" : path;
    }

    inline ImU32 ClassColor(const std::string& cls) {
        if (cls == "Player" || cls == "Players") return IM_COL32(255, 150, 200, 255);
        if (cls == "Model") return IM_COL32(255, 220, 130, 255);
        if (cls == "Humanoid") return IM_COL32(255, 120, 120, 255);
        if (cls == "Script" || cls == "LocalScript" || cls == "ModuleScript") return IM_COL32(255, 175, 90, 255);
        if (cls == "Workspace" || cls == "Lighting" || cls == "ReplicatedStorage" || cls == "ServerStorage" ||
            cls == "StarterPack" || cls == "StarterGui" || cls == "StarterPlayer" || cls == "SoundService" ||
            cls == "Chat" || cls == "HttpService" || cls == "RunService" || cls == "TeleportService" ||
            cls == "UserInputService" || cls == "ContextActionService" || cls == "GuiService" ||
            cls == "Debris" || cls == "TweenService" || cls == "PathfindingService" || cls == "CollectionService" ||
            cls == "ProximityPromptService" || cls == "AvatarEditorService" || cls == "BadgeService" ||
            cls == "MarketplaceService" || cls == "Camera" || cls == "MouseService" ||
            cls == "DataModel") return IM_COL32(125, 205, 255, 255);
        if (cls == "UnionOperation" || cls == "MeshPart" || cls == "Seat" || cls == "VehicleSeat" ||
            cls == "SpawnLocation" || cls == "SkateboardPlatform" || cls == "TrussPart" ||
            cls == "PartOperation") return IM_COL32(145, 255, 145, 255);
        if (cls.size() >= 4 && cls.compare(cls.size() - 4, 4, "Part") == 0) return IM_COL32(145, 255, 145, 255);
        if (cls == "Accessory" || cls == "Hat") return IM_COL32(200, 160, 255, 255);
        if (cls == "Tool" || cls == "HopperBin") return IM_COL32(255, 255, 150, 255);
        if (cls == "Folder") return IM_COL32(180, 180, 180, 255);
        return IM_COL32(230, 230, 230, 255);
    }

    inline bool IsPartClass(const std::string& cls) {
        if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" || cls == "Seat" ||
            cls == "VehicleSeat" || cls == "SpawnLocation" || cls == "SkateboardPlatform" ||
            cls == "TrussPart" || cls == "WedgePart" || cls == "CornerWedgePart" ||
            cls == "PartOperation") return true;
        return (cls.size() >= 4 && cls.compare(cls.size() - 4, 4, "Part") == 0);
    }

    inline void TeleportToPart(uintptr_t partAddr) {
        RBX::RbxInstance part(partAddr);
        RBX::Vec3 tp = part.GetPos();
        if (tp.X == 0 && tp.Y == 0 && tp.Z == 0) return;
        auto ch = Globals::localPlayer.GetModelRef();
        if (ch.Addr == 0) return;
        auto hrp = ch.FindCharacterPart("HumanoidRootPart");
        if (hrp.Addr == 0) return;
        uintptr_t prim = hrp.GetPrimitivePtr();
        if (prim == 0) return;
        tp.Y += 3.0f;
        Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::Position, tp);
        Coms->WriteMemory<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, { 0, 0, 0 });
    }

    // ---------------- tree ----------------
    inline void RenderTreeNode(ExpNode& n) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanFullWidth;
        if (n.addr == selectedAddr) flags |= ImGuiTreeNodeFlags_Selected;
        if (n.fetched && n.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        char label[256];
        if (n.fetched) snprintf(label, sizeof(label), "%s  [%s]  (%d)", n.name.c_str(), n.cls.c_str(), (int)n.children.size());
        else snprintf(label, sizeof(label), "%s  [%s]", n.name.c_str(), n.cls.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, ClassColor(n.cls));
        bool open = ImGui::TreeNodeEx((void*)n.addr, flags, "%s", label);
        ImGui::PopStyleColor();

        if (ImGui::IsItemClicked()) selectedAddr = n.addr;
        if (ImGui::IsItemHovered()) {
            char tip[64];
            snprintf(tip, sizeof(tip), "0x%llX", (unsigned long long)n.addr);
            ImGui::SetTooltip("%s", tip);
        }
        if (open) {
            openSet.insert(n.addr);
            if (!n.fetched) FetchChildren(n);
            for (auto& c : n.children) RenderTreeNode(c);
            ImGui::TreePop();
        }
    }

    inline void RenderHitRow(const FlatHit& h) {
        ImGui::PushID((void*)h.addr);
        char label[256];
        snprintf(label, sizeof(label), "%s  [%s]", h.name.c_str(), h.cls.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ClassColor(h.cls));
        if (ImGui::Selectable(label, h.addr == selectedAddr)) selectedAddr = h.addr;
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
            char tip[64];
            snprintf(tip, sizeof(tip), "0x%llX", (unsigned long long)h.addr);
            ImGui::SetTooltip("%s", tip);
        }
        ImGui::PopID();
    }

    // ---------------- properties ----------------
    inline void RenderProps() {
        ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "Properties");
        ImGui::Separator();
        if (selectedAddr == 0) { ImGui::TextDisabled("Select an instance."); return; }
        RBX::RbxInstance inst(selectedAddr);
        std::string nm = inst.GetName(), cl = inst.GetClass();
        auto kids = inst.GetChildList();
        if (nm.empty() && cl.empty() && kids.empty()) {
            ImGui::TextDisabled("Selected instance is gone.");
            if (ImGui::Button("Clear Selection")) selectedAddr = 0;
            return;
        }
        if (nm.empty()) nm = "Unnamed";
        if (cl.empty()) cl = "???";

        ImGui::Text("Name:"); ImGui::SameLine(110); ImGui::TextWrapped("%s", nm.c_str());
        ImGui::Text("Class:"); ImGui::SameLine(110); ImGui::Text("%s", cl.c_str());
        char abuf[32];
        snprintf(abuf, sizeof(abuf), "0x%llX", (unsigned long long)selectedAddr);
        ImGui::Text("Address:"); ImGui::SameLine(110); ImGui::Text("%s", abuf);
        auto parent = inst.GetParent();
        std::string pnm = parent.Addr ? parent.GetName() : "-";
        if (pnm.empty()) pnm = "Unnamed";
        ImGui::Text("Parent:"); ImGui::SameLine(110); ImGui::TextWrapped("%s", pnm.c_str());
        ImGui::Text("Children:"); ImGui::SameLine(110); ImGui::Text("%d", (int)kids.size());
        ImGui::Text("Path:");
        std::string pth = BuildPath(selectedAddr);
        ImGui::TextWrapped("%s", pth.c_str());
        ImGui::Separator();

        if (IsPartClass(cl)) {
            RBX::Vec3 pos = inst.GetPos();
            ImGui::Text("Position: %.1f, %.1f, %.1f", pos.X, pos.Y, pos.Z);
            uintptr_t prim = inst.GetPrimitivePtr();
            if (prim != 0) {
                float tr = Coms->ReadMemory<float>(selectedAddr + Offsets::BasePart::Transparency);
                float trEdit = tr;
                if (trEdit < 0.0f) trEdit = 0.0f;
                if (trEdit > 1.0f) trEdit = 1.0f;
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderFloat("##transp", &trEdit, 0.0f, 1.0f, "Transparency %.2f"))
                    Coms->WriteMemory<float>(selectedAddr + Offsets::BasePart::Transparency, trEdit);
                DWORD fl = Coms->ReadMemory<DWORD>(prim + Offsets::Primitive::Flags);
                bool cc = (fl & Offsets::PrimitiveFlags::CanCollide) != 0;
                bool an = (fl & Offsets::PrimitiveFlags::Anchored) != 0;
                if (ImGui::Checkbox("CanCollide", &cc)) {
                    DWORD nf = cc ? (fl | Offsets::PrimitiveFlags::CanCollide)
                                  : (fl & ~Offsets::PrimitiveFlags::CanCollide);
                    Coms->WriteMemory<DWORD>(prim + Offsets::Primitive::Flags, nf);
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Anchored", &an)) {
                    DWORD nf = an ? (fl | Offsets::PrimitiveFlags::Anchored)
                                  : (fl & ~Offsets::PrimitiveFlags::Anchored);
                    Coms->WriteMemory<DWORD>(prim + Offsets::Primitive::Flags, nf);
                }
            }
            if (ImGui::Button("Teleport To", ImVec2(-1, 0))) TeleportToPart(selectedAddr);
        }
        else if (cl == "Humanoid") {
            float hp = Coms->ReadMemory<float>(selectedAddr + Offsets::Humanoid::Health);
            float mh = Coms->ReadMemory<float>(selectedAddr + Offsets::Humanoid::MaxHealth);
            float ws = Coms->ReadMemory<float>(selectedAddr + Offsets::Humanoid::Walkspeed);
            ImGui::Text("Health: %.0f / %.0f", hp, mh);
            ImGui::Text("WalkSpeed: %.0f", ws);
        }
        else if (cl == "Player") {
            if (ImGui::Button("Go To Character", ImVec2(-1, 0))) {
                auto ch = inst.GetModelRef();
                if (ch.Addr != 0) selectedAddr = ch.Addr;
            }
        }
        else if (cl == "Model") {
            uintptr_t pp = Coms->ReadMemory<uintptr_t>(selectedAddr + Offsets::Model::PrimaryPart);
            if (pp != 0 && ImGui::Button("Select PrimaryPart", ImVec2(-1, 0))) selectedAddr = pp;
        }
        ImGui::Separator();
        if (ImGui::Button("Copy Address")) {
            char cb[32];
            snprintf(cb, sizeof(cb), "0x%llX", (unsigned long long)selectedAddr);
            ImGui::SetClipboardText(cb);
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy Path")) ImGui::SetClipboardText(pth.c_str());
    }

    // ---------------- main ----------------
    inline void RenderExplorer() {
        if (!Vars::Explorer::enabled) return;

        ImGui::SetNextWindowSize(ImVec2(720, 460), ImGuiCond_FirstUseEver);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, Menu::Bg);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, Menu::DarkAccent);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, Menu::DarkAccent);
        ImGui::PushStyleColor(ImGuiCol_Header, Menu::DarkAccent);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Menu::Accent);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, Menu::Accent);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Menu::InnerBg);
        ImGui::PushStyleColor(ImGuiCol_Button, Menu::DarkAccent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Menu::Accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Menu::Accent);
        ImGui::PushStyleColor(ImGuiCol_Text, Menu::Text);
        ImGui::PushStyleColor(ImGuiCol_Border, Menu::Outline);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Menu::ChildBg);
        ImGui::Begin("Explorer", &Vars::Explorer::enabled);

        if (Globals::dataModel.Addr == 0) {
            ImGui::TextDisabled("DataModel not loaded...");
            ImGui::End();
            ImGui::PopStyleColor(13);
            return;
        }
        if (!rootInit || root.addr != Globals::dataModel.Addr) {
            root = ExpNode{};
            root.addr = Globals::dataModel.Addr;
            root.name = "Game";
            root.cls = "DataModel";
            FetchChildren(root);
            rootInit = true;
            openSet.clear();
            deepHits.clear();
            deepDone = false;
            deepScanned = 0;
        }

        auto now = std::chrono::steady_clock::now();
        if (autoRefresh && std::chrono::duration<float>(now - lastRef).count() > 1.0f) {
            RefreshOpen(root);
            lastRef = now;
        }
        openSet.clear();

        if (ImGui::Button("Refresh")) {
            RefreshOpen(root);
            lastRef = std::chrono::steady_clock::now();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto", &autoRefresh);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
        ImGui::InputTextWithHint("##expsearch", "Search name or class...", searchBuf, sizeof(searchBuf));
        std::string query = ToLower(searchBuf);
        bool searching = !query.empty();
        if (searching) {
            ImGui::SameLine();
            if (ImGui::Button("Deep")) {
                deepHits.clear();
                deepScanned = 0;
                deepDone = false;
                DeepScan(root.addr, query, 0);
                deepDone = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                searchBuf[0] = '\0';
                deepHits.clear();
                deepDone = false;
                deepScanned = 0;
            }
        }

        ImGui::Separator();
        float treeW = ImGui::GetContentRegionAvail().x * 0.58f;
        ImGui::BeginChild("ExpTree", ImVec2(treeW, 0), true);
        if (searching) {
            searchHits.clear();
            CollectMatches(root, query, searchHits);
            ImGui::TextDisabled("Loaded (%d)%s", (int)searchHits.size(), searchHits.size() >= 300 ? " [capped]" : "");
            for (auto& h : searchHits) RenderHitRow(h);
            ImGui::Separator();
            if (!deepDone) {
                ImGui::TextDisabled("Press Deep for a full-tree scan.");
            }
            else {
                ImGui::TextDisabled("Deep (%d of %d scanned)%s", (int)deepHits.size(), deepScanned,
                    deepHits.size() >= 500 ? " [capped]" : "");
                for (auto& h : deepHits) RenderHitRow(h);
            }
        }
        else {
            deepHits.clear();
            deepDone = false;
            deepScanned = 0;
            RenderTreeNode(root);
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("ExpProps", ImVec2(0, 0), true);
        RenderProps();
        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleColor(13);
    }
}
