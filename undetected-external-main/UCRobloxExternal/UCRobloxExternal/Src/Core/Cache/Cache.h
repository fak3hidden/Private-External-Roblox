#pragma once
#include "../../Game/SDK/SDK.h"
#include "../Globals/Globals.h"
#include <vector>
#include <string>
#include <algorithm>

namespace PlayerCache {

    struct CachedPlayer {
        uintptr_t playerAddr    = 0;
        uintptr_t characterAddr = 0;
        uintptr_t humanoidAddr  = 0;
        uintptr_t rootPartAddr  = 0;

        std::string name;
        std::string displayName;
        RBX::Vec3 position;
        float health    = 0.f;
        float maxHealth = 0.f;
        float distance  = 0.f;

        uintptr_t teamAddr  = 0;
        int       teamColor = -1;
        bool      isTeammate = false;

        bool isValid = false;
    };

    inline std::vector<CachedPlayer> players;
    inline RBX::Vec3 localPlayerPos;
    inline int debugRawCount = 0; // raw children of the Players service (watermark debug)
    inline int debugStage = 0;    // UpdatePlayers progress: 1=local done, 2=players pass, 3=chars pass

    inline void UpdatePlayers() {
        debugStage = 0;
        if (Globals::players.Addr == 0 || Globals::workspace.Addr == 0) {
            players.clear();
            debugRawCount = 0;
            return;
        }

        // --- Local reference (a failure here must NOT wipe out remote players) ---
        auto localChar = Globals::localPlayer.GetModelRef();
        uintptr_t localCharAddr = localChar.Addr;
        {
            auto localRoot = localChar.IsValid() ? localChar.FindCharacterPart("HumanoidRootPart") : RBX::RbxInstance(0);
            if (localRoot.Addr == 0 && localChar.IsValid()) localRoot = localChar.FindCharacterPart("Root");
            if (localRoot.IsValid()) localPlayerPos = localRoot.GetPos();
            else if (Globals::camera.IsValid()) localPlayerPos = Globals::camera.GetCameraPosition();
        }
        debugStage = 1;

        for (auto& cached : players) cached.isValid = false;

        // --- Source 1: Players service ---
        auto playerList = Globals::players.GetChildList();
        debugRawCount = static_cast<int>(playerList.size());

        for (auto& plr : playerList) {
            if (!plr.IsValid()) continue;
            if (plr.Addr == Globals::localPlayer.Addr) continue;
            if (plr.GetClass() != "Player") continue;

            size_t idx = players.size();
            for (size_t i = 0; i < players.size(); ++i) {
                if (players[i].playerAddr == plr.Addr) { idx = i; break; }
            }
            if (idx == players.size()) {
                CachedPlayer np;
                np.playerAddr = plr.Addr;
                np.name = plr.GetName();
                uintptr_t dispPtr = Coms->ReadMemory<uintptr_t>(plr.Addr + Offsets::Player::DisplayName);
                np.displayName = dispPtr ? Coms->ReadGameString(plr.Addr + Offsets::Player::DisplayName) : "";
                players.push_back(np);
            }
            CachedPlayer& entry = players[idx];

            auto character = plr.GetModelRef();
            if (!character.IsValid()) continue;
            entry.characterAddr = character.Addr;

            auto humanoid = character.FindChildByClass("Humanoid");
            entry.humanoidAddr = humanoid.Addr;

            auto rootPart = character.FindCharacterPart("HumanoidRootPart");
            if (rootPart.Addr == 0) rootPart = character.FindCharacterPart("Root");
            if (rootPart.Addr == 0) {
                // Last resort: the model's PrimaryPart.
                uintptr_t pp = Coms->ReadMemory<uintptr_t>(character.Addr + Offsets::Model::PrimaryPart);
                if (pp != 0) rootPart = RBX::RbxInstance(pp);
            }
            if (rootPart.Addr == 0) continue;

            entry.rootPartAddr = rootPart.Addr;
            entry.position = rootPart.GetPos();

            if (humanoid.Addr != 0) {
                entry.health = Coms->ReadMemory<float>(humanoid.Addr + Offsets::Humanoid::Health);
                entry.maxHealth = Coms->ReadMemory<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
            } else {
                auto healthVal = character.FindChild("Health");
                if (healthVal.Addr != 0) {
                    entry.health = RBX::GetNumberValue(healthVal);
                    entry.maxHealth = 100.0f;
                } else {
                    entry.health = 100.0f;
                    entry.maxHealth = 100.0f;
                }
            }

            entry.distance = rootPart.CalcDistance(localPlayerPos);

            uintptr_t localTeam = Coms->ReadMemory<uintptr_t>(Globals::localPlayer.Addr + Offsets::Player::Team);
            uintptr_t targetTeam = Coms->ReadMemory<uintptr_t>(plr.Addr + Offsets::Player::Team);
            entry.teamAddr = targetTeam;
            if (localTeam != 0 && targetTeam != 0) {
                if (localTeam == targetTeam) {
                    entry.isTeammate = true;
                } else {
                    int localColor = Coms->ReadMemory<int>(localTeam + Offsets::Team::BrickColor);
                    int targetColor = Coms->ReadMemory<int>(targetTeam + Offsets::Team::BrickColor);
                    entry.teamColor = targetColor;
                    entry.isTeammate = (localColor == targetColor);
                }
            } else {
                entry.teamColor = -1;
                entry.isTeammate = false;
            }

            entry.isValid = true;
        }
        debugStage = 2;

        // --- Source 2: Workspace/Characters (Bad Business style games) ---
        // Merged with source 1 and deduped by character address.
        auto charactersFolder = Globals::workspace.FindChild("Characters");
        if (charactersFolder.IsValid()) {
            for (auto& character : charactersFolder.GetChildList()) {
                if (!character.IsValid()) continue;

                bool dup = false;
                for (auto& c : players) {
                    if (c.isValid && c.characterAddr == character.Addr) { dup = true; break; }
                }
                if (dup) continue;
                if (localCharAddr != 0 && character.Addr == localCharAddr) continue; // local player

                auto rootPart = character.FindCharacterPart("HumanoidRootPart");
                if (rootPart.Addr == 0) rootPart = character.FindCharacterPart("Root");
                if (rootPart.Addr == 0) continue;

                float dist = rootPart.CalcDistance(localPlayerPos);
                if (localCharAddr == 0 && dist < 4.0f) continue; // legacy local-skip when local char unknown

                size_t idx = players.size();
                for (size_t i = 0; i < players.size(); ++i) {
                    if (players[i].playerAddr == character.Addr) { idx = i; break; }
                }
                if (idx == players.size()) {
                    CachedPlayer np;
                    np.playerAddr = character.Addr;
                    np.name = character.GetName();
                    np.displayName = np.name;
                    players.push_back(np);
                }
                CachedPlayer& entry = players[idx];
                entry.characterAddr = character.Addr;
                entry.rootPartAddr = rootPart.Addr;
                entry.position = rootPart.GetPos();
                entry.distance = dist;

                auto healthVal = character.FindChild("Health");
                if (healthVal.Addr != 0) {
                    entry.health = RBX::GetNumberValue(healthVal);
                    entry.maxHealth = 100.0f;
                } else {
                    entry.health = 100.0f;
                    entry.maxHealth = 100.0f;
                }

                entry.isTeammate = false;
                entry.isValid = true;
            }
        }
        debugStage = 3;

        players.erase(
            std::remove_if(players.begin(), players.end(),
                [](const CachedPlayer& p) { return !p.isValid; }),
            players.end()
        );
    }
}
