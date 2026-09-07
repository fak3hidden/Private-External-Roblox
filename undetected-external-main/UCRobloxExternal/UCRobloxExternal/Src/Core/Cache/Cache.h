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

    inline void UpdatePlayers() {
        auto charactersFolder = Globals::workspace.FindChild("Characters");
        if (charactersFolder.Addr != 0) {
            players.clear();
            auto charList = charactersFolder.GetChildList();
            localPlayerPos = Globals::camera.GetCameraPosition();

            for (auto& character : charList) {
                if (character.Addr == 0) continue;
                
                auto rootPart = character.FindCharacterPart("HumanoidRootPart");
                if (rootPart.Addr == 0) rootPart = character.FindCharacterPart("Root");
                if (rootPart.Addr == 0) continue;

                float dist = rootPart.CalcDistance(localPlayerPos);
                if (dist < 4.0f) continue; // Skip local player character

                CachedPlayer p;
                p.playerAddr = character.Addr;
                p.characterAddr = character.Addr;
                p.name = character.GetName();
                p.displayName = character.GetName();
                p.rootPartAddr = rootPart.Addr;
                p.position = rootPart.GetPos();
                p.distance = dist;
                p.isValid = true;

                auto healthVal = character.FindChild("Health");
                if (healthVal.Addr != 0) {
                    p.health = RBX::GetNumberValue(healthVal);
                    p.maxHealth = 100.0f;
                } else {
                    p.health = 100.0f;
                    p.maxHealth = 100.0f;
                }

                p.isTeammate = false; 

                players.push_back(p);
            }
            return;
        }

        auto playerList = Globals::players.GetChildList();

        auto localChar = Globals::localPlayer.GetModelRef();
        if (localChar.Addr == 0) return;

        auto localRoot = localChar.FindCharacterPart("HumanoidRootPart");
        if (localRoot.Addr == 0) localRoot = localChar.FindCharacterPart("Root");
        if (localRoot.Addr == 0) return;
        
        localPlayerPos = localRoot.GetPos();

        for(auto& cached : players){
            cached.isValid = false;
        }

        for(auto& plr : playerList){
            if(plr.Addr == Globals::localPlayer.Addr) continue;

            CachedPlayer* existingPlayer = nullptr;
            for(auto& cached : players){
                if(cached.playerAddr == plr.Addr){
                    existingPlayer = &cached;
                    break;
                }
            }

            if(existingPlayer == nullptr){
                CachedPlayer newPlayer;
                newPlayer.playerAddr = plr.Addr;
                newPlayer.name = plr.GetName();
                newPlayer.displayName = Coms->ReadMemory<uintptr_t>(plr.Addr + Offsets::Player::DisplayName) ? Coms->ReadGameString(plr.Addr + Offsets::Player::DisplayName) : "";
                players.push_back(newPlayer);
                existingPlayer = &players.back();
            }

            auto character = plr.GetModelRef();
            if(character.Addr == 0) continue;
            
            existingPlayer->characterAddr = character.Addr;

            auto humanoid = character.FindChildByClass("Humanoid");
            existingPlayer->humanoidAddr = humanoid.Addr;

            auto rootPart = character.FindCharacterPart("HumanoidRootPart");
            if (rootPart.Addr == 0) rootPart = character.FindCharacterPart("Root");
            if (rootPart.Addr == 0) continue;

            existingPlayer->rootPartAddr = rootPart.Addr;
            existingPlayer->position     = rootPart.GetPos();

            if (humanoid.Addr != 0) {
                existingPlayer->health    = Coms->ReadMemory<float>(humanoid.Addr + Offsets::Humanoid::Health);
                existingPlayer->maxHealth = Coms->ReadMemory<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
            } else {
                auto healthVal = character.FindChild("Health");
                if (healthVal.Addr != 0) {
                    existingPlayer->health = RBX::GetNumberValue(healthVal);
                    existingPlayer->maxHealth = 100.0f; // Bad Business standard max
                } else {
                    existingPlayer->health = 100.0f;
                    existingPlayer->maxHealth = 100.0f;
                }
            }
            
            existingPlayer->distance     = rootPart.CalcDistance(localPlayerPos);

            // Team check — pointer equality is the reliable primary check
            uintptr_t localTeam  = Coms->ReadMemory<uintptr_t>(Globals::localPlayer.Addr + Offsets::Player::Team);
            uintptr_t targetTeam = Coms->ReadMemory<uintptr_t>(plr.Addr + Offsets::Player::Team);
            existingPlayer->teamAddr = targetTeam;
            if (localTeam != 0 && targetTeam != 0) {
                if (localTeam == targetTeam) {
                    // Same Team instance — definitely teammates
                    existingPlayer->isTeammate = true;
                } else {
                    // Different Team instances — compare BrickColor as fallback
                    int localColor  = Coms->ReadMemory<int>(localTeam  + Offsets::Team::BrickColor);
                    int targetColor = Coms->ReadMemory<int>(targetTeam + Offsets::Team::BrickColor);
                    existingPlayer->teamColor  = targetColor;
                    existingPlayer->isTeammate = (localColor == targetColor);
                }
            } else {
                existingPlayer->teamColor  = -1;
                existingPlayer->isTeammate = false;
            }


            existingPlayer->isValid = true;
        }

        players.erase(
            std::remove_if(players.begin(), players.end(),
                [](const CachedPlayer& p) { return !p.isValid; }),
            players.end()
        );

    }
}
