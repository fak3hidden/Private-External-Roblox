#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <Windows.h>
#include "../Vars/Vars.h"

namespace Config {

    static const std::string CONFIG_EXT = ".nxghtcfg";
    static const std::string CONFIG_DIR = "configs\\";

    // ── Base64 ──────────────────────────────────────────────────────────────
    static const std::string B64_CHARS =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static std::string Base64Encode(const std::string& in) {
        std::string out;
        int val = 0, bits = -6;
        unsigned char mask = 0x3F;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            bits += 8;
            while (bits >= 0) {
                out.push_back(B64_CHARS[(val >> bits) & mask]);
                bits -= 6;
            }
        }
        if (bits > -6) out.push_back(B64_CHARS[((val << 8) >> (bits + 8)) & mask]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    static std::string Base64Decode(const std::string& in) {
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[(unsigned char)B64_CHARS[i]] = i;
        int val = 0, bits = -8;
        for (unsigned char c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            bits += 6;
            if (bits >= 0) {
                out.push_back(char((val >> bits) & 0xFF));
                bits -= 8;
            }
        }
        return out;
    }

    // ── Minimal JSON helpers ─────────────────────────────────────────────────
    static std::string BoolStr(bool v) { return v ? "true" : "false"; }
    static std::string FltStr(float v) { std::ostringstream ss; ss << v; return ss.str(); }
    static std::string IntStr(int v)   { return std::to_string(v); }

    static std::string BuildJson() {
        std::ostringstream j;
        j << "{\n";

        // Aimbot
        j << "  \"aimbot_enabled\":"          << BoolStr(Vars::Aimbot::enabled)        << ",\n";
        j << "  \"aimbot_showFOV\":"           << BoolStr(Vars::Aimbot::showFOV)        << ",\n";
        j << "  \"aimbot_fovRadius\":"         << FltStr(Vars::Aimbot::fovRadius)       << ",\n";
        j << "  \"aimbot_smoothing\":"         << FltStr(Vars::Aimbot::smoothing)       << ",\n";
        j << "  \"aimbot_aimTarget\":"         << IntStr(Vars::Aimbot::aimTarget)       << ",\n";
        j << "  \"aimbot_aimMethod\":"         << IntStr(Vars::Aimbot::aimMethod)       << ",\n";
        j << "  \"aimbot_key\":"               << IntStr(Vars::Aimbot::aimbotKey)       << ",\n";
        j << "  \"aimbot_teamCheck\":"         << BoolStr(Vars::Aimbot::teamCheck)      << ",\n";
        j << "  \"aimbot_downedCheck\":"       << BoolStr(Vars::Aimbot::downedCheck)    << ",\n";
        j << "  \"aimbot_invisCheck\":"        << BoolStr(Vars::Aimbot::invisCheck)     << ",\n";
        j << "  \"aimbot_prediction\":"        << BoolStr(Vars::Aimbot::prediction)     << ",\n";
        j << "  \"aimbot_predX\":"             << FltStr(Vars::Aimbot::predX)           << ",\n";
        j << "  \"aimbot_predY\":"             << FltStr(Vars::Aimbot::predY)           << ",\n";
        j << "  \"aimbot_silentAim\":"         << BoolStr(Vars::Aimbot::silentAim)      << ",\n";
        j << "  \"aimbot_showSilentFOV\":"     << BoolStr(Vars::Aimbot::showSilentFOV)  << ",\n";
        j << "  \"aimbot_silentFOV\":"         << FltStr(Vars::Aimbot::silentFOV)       << ",\n";
        j << "  \"aimbot_silentKey\":"         << IntStr(Vars::Aimbot::silentAimbotKey) << ",\n";
        j << "  \"aimbot_silentPred\":"        << BoolStr(Vars::Aimbot::silentPrediction)<< ",\n";
        j << "  \"aimbot_randomTarget\":"       << BoolStr(Vars::Aimbot::randomTarget)    << ",\n";

        // Triggerbot
        j << "  \"trig_enabled\":"              << BoolStr(Vars::Triggerbot::enabled)      << ",\n";
        j << "  \"trig_key\":"                  << IntStr (Vars::Triggerbot::key)           << ",\n";
        j << "  \"trig_delay\":"                << FltStr (Vars::Triggerbot::delay)         << ",\n";
        j << "  \"trig_fov\":"                  << FltStr (Vars::Triggerbot::fov)           << ",\n";
        j << "  \"trig_teamCheck\":"            << BoolStr(Vars::Triggerbot::teamCheck)    << ",\n";

        // ESP
        j << "  \"esp_enabled\":"             << BoolStr(Vars::ESP::enabled)           << ",\n";
        j << "  \"esp_boxMode\":"             << IntStr(Vars::ESP::boxMode)            << ",\n";
        j << "  \"esp_nameMode\":"            << IntStr(Vars::ESP::nameMode)           << ",\n";
        j << "  \"esp_distance\":"            << BoolStr(Vars::ESP::distance)          << ",\n";
        j << "  \"esp_healthBar\":"           << BoolStr(Vars::ESP::healthBar)         << ",\n";
        j << "  \"esp_healthText\":"          << BoolStr(Vars::ESP::healthText)        << ",\n";
        j << "  \"esp_tracers\":"             << BoolStr(Vars::ESP::tracers)           << ",\n";
        j << "  \"esp_tracerOrigin\":"        << IntStr(Vars::ESP::tracerOrigin)        << ",\n";
        j << "  \"esp_offscreen\":"           << BoolStr(Vars::ESP::offscreen)         << ",\n";
        j << "  \"esp_skeleton\":"            << BoolStr(Vars::ESP::skeleton)          << ",\n";
        j << "  \"esp_filledBox\":"           << BoolStr(Vars::ESP::filledBox)         << ",\n";
        j << "  \"esp_fillR\":"               << FltStr(Vars::ESP::filledColor[0])     << ",\n";
        j << "  \"esp_fillG\":"               << FltStr(Vars::ESP::filledColor[1])     << ",\n";
        j << "  \"esp_fillB\":"               << FltStr(Vars::ESP::filledColor[2])     << ",\n";
        j << "  \"esp_fillA\":"               << FltStr(Vars::ESP::filledColor[3])     << ",\n";
        j << "  \"esp_crosshair\":"           << BoolStr(Vars::ESP::crosshair)         << ",\n";
        j << "  \"esp_crossSize\":"           << FltStr(Vars::ESP::crosshairSize)      << ",\n";
        j << "  \"esp_crossThick\":"          << FltStr(Vars::ESP::crosshairThickness)<< ",\n";
        j << "  \"esp_crossGap\":"            << FltStr(Vars::ESP::crosshairGap)       << ",\n";
        j << "  \"esp_crossDot\":"            << BoolStr(Vars::ESP::crosshairDot)      << ",\n";
        j << "  \"esp_crossR\":"              << FltStr(Vars::ESP::crosshairColor[0])  << ",\n";
        j << "  \"esp_crossG\":"              << FltStr(Vars::ESP::crosshairColor[1])  << ",\n";
        j << "  \"esp_crossB\":"              << FltStr(Vars::ESP::crosshairColor[2])  << ",\n";
        j << "  \"esp_crossA\":"              << FltStr(Vars::ESP::crosshairColor[3])  << ",\n";
        j << "  \"esp_headDot\":"             << BoolStr(Vars::ESP::headDot)           << ",\n";
        j << "  \"esp_viewAngle\":"           << BoolStr(Vars::ESP::viewAngle)         << ",\n";
        j << "  \"esp_teamCheck\":"            << BoolStr(Vars::ESP::teamCheck)         << ",\n";
        j << "  \"esp_staticBox\":"           << BoolStr(Vars::ESP::staticBox)         << ",\n";
        j << "  \"esp_textGrad\":"            << BoolStr(Vars::ESP::textGradient)      << ",\n";
        j << "  \"esp_textGradR\":"           << FltStr(Vars::ESP::textGradientColor[0])<< ",\n";
        j << "  \"esp_textGradG\":"           << FltStr(Vars::ESP::textGradientColor[1])<< ",\n";
        j << "  \"esp_textGradB\":"           << FltStr(Vars::ESP::textGradientColor[2])<< ",\n";
        j << "  \"esp_textGradA\":"           << FltStr(Vars::ESP::textGradientColor[3])<< ",\n";
        j << "  \"esp_boxR\":"                << FltStr(Vars::ESP::boxColor[0])        << ",\n";
        j << "  \"esp_boxG\":"                << FltStr(Vars::ESP::boxColor[1])        << ",\n";
        j << "  \"esp_boxB\":"                << FltStr(Vars::ESP::boxColor[2])        << ",\n";
        j << "  \"esp_boxA\":"                << FltStr(Vars::ESP::boxColor[3])        << ",\n";
        j << "  \"esp_boxThick\":"            << FltStr(Vars::ESP::boxThickness)       << ",\n";
        j << "  \"esp_boxGrad\":"             << BoolStr(Vars::ESP::boxGradient)       << ",\n";
        j << "  \"esp_boxGradR\":"            << FltStr(Vars::ESP::boxGradientColor[0])<< ",\n";
        j << "  \"esp_boxGradG\":"            << FltStr(Vars::ESP::boxGradientColor[1])<< ",\n";
        j << "  \"esp_boxGradB\":"            << FltStr(Vars::ESP::boxGradientColor[2])<< ",\n";
        j << "  \"esp_boxGradA\":"            << FltStr(Vars::ESP::boxGradientColor[3])<< ",\n";
        j << "  \"esp_skelR\":"               << FltStr(Vars::ESP::skeletonColor[0])   << ",\n";
        j << "  \"esp_skelG\":"               << FltStr(Vars::ESP::skeletonColor[1])   << ",\n";
        j << "  \"esp_skelB\":"               << FltStr(Vars::ESP::skeletonColor[2])   << ",\n";
        j << "  \"esp_skelA\":"               << FltStr(Vars::ESP::skeletonColor[3])   << ",\n";
        j << "  \"esp_skelThick\":"           << FltStr(Vars::ESP::skeletonThickness)  << ",\n";
        j << "  \"esp_nameR\":"               << FltStr(Vars::ESP::nameColor[0])       << ",\n";
        j << "  \"esp_nameG\":"               << FltStr(Vars::ESP::nameColor[1])       << ",\n";
        j << "  \"esp_nameB\":"               << FltStr(Vars::ESP::nameColor[2])       << ",\n";
        j << "  \"esp_nameA\":"               << FltStr(Vars::ESP::nameColor[3])       << ",\n";
        j << "  \"esp_distR\":"               << FltStr(Vars::ESP::distanceColor[0])   << ",\n";
        j << "  \"esp_distG\":"               << FltStr(Vars::ESP::distanceColor[1])   << ",\n";
        j << "  \"esp_distB\":"               << FltStr(Vars::ESP::distanceColor[2])   << ",\n";
        j << "  \"esp_distA\":"               << FltStr(Vars::ESP::distanceColor[3])   << ",\n";
        j << "  \"esp_traceR\":"              << FltStr(Vars::ESP::tracerColor[0])     << ",\n";
        j << "  \"esp_traceG\":"              << FltStr(Vars::ESP::tracerColor[1])     << ",\n";
        j << "  \"esp_traceB\":"              << FltStr(Vars::ESP::tracerColor[2])     << ",\n";
        j << "  \"esp_traceA\":"              << FltStr(Vars::ESP::tracerColor[3])     << ",\n";
        j << "  \"esp_hDotR\":"               << FltStr(Vars::ESP::headDotColor[0])    << ",\n";
        j << "  \"esp_hDotG\":"               << FltStr(Vars::ESP::headDotColor[1])    << ",\n";
        j << "  \"esp_hDotB\":"               << FltStr(Vars::ESP::headDotColor[2])    << ",\n";
        j << "  \"esp_hDotA\":"               << FltStr(Vars::ESP::headDotColor[3])    << ",\n";

        // Local
        j << "  \"local_speedEnabled\":"      << BoolStr(Vars::Local::speedEnabled)    << ",\n";
        j << "  \"local_walkSpeed\":"         << FltStr(Vars::Local::walkSpeed)        << ",\n";
        j << "  \"local_jumpEnabled\":"       << BoolStr(Vars::Local::jumpEnabled)     << ",\n";
        j << "  \"local_jumpPower\":"         << FltStr(Vars::Local::jumpPower)        << ",\n";
        j << "  \"local_flightEnabled\":"     << BoolStr(Vars::Local::flightEnabled)   << ",\n";
        j << "  \"local_flightSpeed\":"       << FltStr(Vars::Local::flightSpeed)      << ",\n";
        j << "  \"local_flightKey\":"         << IntStr(Vars::Local::flightKey)         << ",\n";
        j << "  \"local_noclipEnabled\":"     << BoolStr(Vars::Local::noclipEnabled)   << ",\n";
        j << "  \"local_noclipKey\":"         << IntStr(Vars::Local::noclipKey)        << ",\n";
        j << "  \"local_infiniteJump\":"      << BoolStr(Vars::Local::infiniteJumpEnabled)<< ",\n";
        j << "  \"local_desyncEnabled\":"     << BoolStr(Vars::Local::desyncEnabled)   << ",\n";
        j << "  \"local_desyncKey\":"         << IntStr(Vars::Local::desyncKey)        << ",\n";
        j << "  \"local_desyncType\":"        << IntStr(Vars::Local::desyncType)       << ",\n";
        j << "  \"local_serverMode\":"        << IntStr(Vars::Local::serverMode)       << ",\n";
        j << "  \"local_clientMode\":"        << IntStr(Vars::Local::clientMode)       << ",\n";

        // Misc
        j << "  \"misc_streamProof\":"        << BoolStr(Vars::Misc::streamProof)      << ",\n";

        // World
        j << "  \"world_skybox\":"            << BoolStr(Vars::World::skybox)          << ",\n";
        j << "  \"world_skyboxType\":"        << IntStr(Vars::World::skyboxType)        << ",\n";
        j << "  \"world_rotateSkybox\":"      << BoolStr(Vars::World::rotateSkybox)    << ",\n";
        j << "  \"world_rotateSpeed\":"       << FltStr(Vars::World::rotateSpeed)      << ",\n";
        j << "  \"world_ambience\":"          << BoolStr(Vars::World::ambience)        << ",\n";
        j << "  \"world_fog\":"               << BoolStr(Vars::World::fog)             << ",\n";
        j << "  \"world_fogDistance\":"       << FltStr(Vars::World::fogDistance)      << ",\n";
        j << "  \"world_brightness\":"        << BoolStr(Vars::World::brightness)      << ",\n";
        j << "  \"world_brightnessValue\":"   << FltStr(Vars::World::brightnessValue)  << ",\n";
        j << "  \"world_exposure\":"          << BoolStr(Vars::World::exposure)        << ",\n";
        j << "  \"world_exposureValue\":"     << FltStr(Vars::World::exposureValue)    << ",\n";
        j << "  \"world_fov\":"               << BoolStr(Vars::World::fov)             << ",\n";
        j << "  \"world_fovValue\":"          << FltStr(Vars::World::fovValue)         << "\n";

        j << "}\n";
        return j.str();
    }

    // ── Tiny key-value parser (no external deps) ─────────────────────────────
    static std::string GetValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":";
        auto pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n')) pos++;
        auto end = json.find_first_of(",\n}", pos);
        if (end == std::string::npos) end = json.size();
        std::string val = json.substr(pos, end - pos);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\r')) val.pop_back();
        return val;
    }

    static bool ParseBool(const std::string& json, const std::string& key, bool fallback) {
        auto v = GetValue(json, key);
        if (v == "true")  return true;
        if (v == "false") return false;
        return fallback;
    }

    static float ParseFloat(const std::string& json, const std::string& key, float fallback) {
        auto v = GetValue(json, key);
        if (v.empty()) return fallback;
        try { return std::stof(v); } catch (...) { return fallback; }
    }

    static int ParseInt(const std::string& json, const std::string& key, int fallback) {
        auto v = GetValue(json, key);
        if (v.empty()) return fallback;
        try { return std::stoi(v); } catch (...) { return fallback; }
    }

    static void ApplyJson(const std::string& json) {
        Vars::Aimbot::enabled           = ParseBool (json, "aimbot_enabled",       Vars::Aimbot::enabled);
        Vars::Aimbot::showFOV           = ParseBool (json, "aimbot_showFOV",       Vars::Aimbot::showFOV);
        Vars::Aimbot::fovRadius         = ParseFloat(json, "aimbot_fovRadius",     Vars::Aimbot::fovRadius);
        Vars::Aimbot::smoothing         = ParseFloat(json, "aimbot_smoothing",     Vars::Aimbot::smoothing);
        Vars::Aimbot::aimTarget         = ParseInt  (json, "aimbot_aimTarget",     Vars::Aimbot::aimTarget);
        Vars::Aimbot::aimMethod         = ParseInt  (json, "aimbot_aimMethod",     Vars::Aimbot::aimMethod);
        Vars::Aimbot::aimbotKey         = ParseInt  (json, "aimbot_key",           Vars::Aimbot::aimbotKey);
        Vars::Aimbot::teamCheck         = ParseBool (json, "aimbot_teamCheck",     Vars::Aimbot::teamCheck);
        Vars::Aimbot::downedCheck       = ParseBool (json, "aimbot_downedCheck",   Vars::Aimbot::downedCheck);
        Vars::Aimbot::invisCheck        = ParseBool (json, "aimbot_invisCheck",    Vars::Aimbot::invisCheck);
        Vars::Aimbot::prediction        = ParseBool (json, "aimbot_prediction",    Vars::Aimbot::prediction);
        Vars::Aimbot::predX             = ParseFloat(json, "aimbot_predX",         Vars::Aimbot::predX);
        Vars::Aimbot::predY             = ParseFloat(json, "aimbot_predY",         Vars::Aimbot::predY);
        Vars::Aimbot::silentAim         = ParseBool (json, "aimbot_silentAim",     Vars::Aimbot::silentAim);
        Vars::Aimbot::showSilentFOV     = ParseBool (json, "aimbot_showSilentFOV", Vars::Aimbot::showSilentFOV);
        Vars::Aimbot::silentFOV         = ParseFloat(json, "aimbot_silentFOV",     Vars::Aimbot::silentFOV);
        Vars::Aimbot::silentAimbotKey   = ParseInt  (json, "aimbot_silentKey",     Vars::Aimbot::silentAimbotKey);
        Vars::Aimbot::silentPrediction  = ParseBool (json, "aimbot_silentPred",    Vars::Aimbot::silentPrediction);
        Vars::Aimbot::randomTarget      = ParseBool (json, "aimbot_randomTarget",  Vars::Aimbot::randomTarget);

        Vars::Triggerbot::enabled   = ParseBool (json, "trig_enabled",   Vars::Triggerbot::enabled);
        Vars::Triggerbot::key       = ParseInt  (json, "trig_key",       Vars::Triggerbot::key);
        Vars::Triggerbot::delay     = ParseFloat(json, "trig_delay",     Vars::Triggerbot::delay);
        Vars::Triggerbot::fov       = ParseFloat(json, "trig_fov",       Vars::Triggerbot::fov);
        Vars::Triggerbot::teamCheck = ParseBool (json, "trig_teamCheck", Vars::Triggerbot::teamCheck);

        Vars::ESP::enabled      = ParseBool (json, "esp_enabled",       Vars::ESP::enabled);
        Vars::ESP::boxMode      = ParseInt  (json, "esp_boxMode",       Vars::ESP::boxMode);
        Vars::ESP::nameMode     = ParseInt  (json, "esp_nameMode",      Vars::ESP::nameMode);
        Vars::ESP::distance     = ParseBool (json, "esp_distance",      Vars::ESP::distance);
        Vars::ESP::healthBar    = ParseBool (json, "esp_healthBar",     Vars::ESP::healthBar);
        Vars::ESP::healthText   = ParseBool (json, "esp_healthText",    Vars::ESP::healthText);
        Vars::ESP::tracers      = ParseBool (json, "esp_tracers",       Vars::ESP::tracers);
        Vars::ESP::tracerOrigin = ParseInt  (json, "esp_tracerOrigin",  Vars::ESP::tracerOrigin);
        Vars::ESP::offscreen    = ParseBool (json, "esp_offscreen",     Vars::ESP::offscreen);
        Vars::ESP::skeleton     = ParseBool (json, "esp_skeleton",      Vars::ESP::skeleton);
        Vars::ESP::filledBox    = ParseBool (json, "esp_filledBox",     Vars::ESP::filledBox);
        Vars::ESP::filledColor[0] = ParseFloat(json, "esp_fillR",      Vars::ESP::filledColor[0]);
        Vars::ESP::filledColor[1] = ParseFloat(json, "esp_fillG",      Vars::ESP::filledColor[1]);
        Vars::ESP::filledColor[2] = ParseFloat(json, "esp_fillB",      Vars::ESP::filledColor[2]);
        Vars::ESP::filledColor[3] = ParseFloat(json, "esp_fillA",      Vars::ESP::filledColor[3]);
        Vars::ESP::crosshair      = ParseBool(json, "esp_crosshair",    Vars::ESP::crosshair);
        Vars::ESP::crosshairSize  = ParseFloat(json, "esp_crossSize",   Vars::ESP::crosshairSize);
        Vars::ESP::crosshairThickness = ParseFloat(json, "esp_crossThick",Vars::ESP::crosshairThickness);
        Vars::ESP::crosshairGap   = ParseFloat(json, "esp_crossGap",    Vars::ESP::crosshairGap);
        Vars::ESP::crosshairDot   = ParseBool (json, "esp_crossDot",    Vars::ESP::crosshairDot);
        Vars::ESP::crosshairColor[0] = ParseFloat(json, "esp_crossR",  Vars::ESP::crosshairColor[0]);
        Vars::ESP::crosshairColor[1] = ParseFloat(json, "esp_crossG",  Vars::ESP::crosshairColor[1]);
        Vars::ESP::crosshairColor[2] = ParseFloat(json, "esp_crossB",  Vars::ESP::crosshairColor[2]);
        Vars::ESP::crosshairColor[3] = ParseFloat(json, "esp_crossA",  Vars::ESP::crosshairColor[3]);
        Vars::ESP::headDot        = ParseBool (json, "esp_headDot",     Vars::ESP::headDot);
        Vars::ESP::viewAngle      = ParseBool (json, "esp_viewAngle",   Vars::ESP::viewAngle);
        Vars::ESP::teamCheck      = ParseBool (json, "esp_teamCheck",   Vars::ESP::teamCheck);
        Vars::ESP::staticBox      = ParseBool (json, "esp_staticBox",   Vars::ESP::staticBox);
        Vars::ESP::textGradient   = ParseBool (json, "esp_textGrad",    Vars::ESP::textGradient);
        Vars::ESP::textGradientColor[0] = ParseFloat(json, "esp_textGradR", Vars::ESP::textGradientColor[0]);
        Vars::ESP::textGradientColor[1] = ParseFloat(json, "esp_textGradG", Vars::ESP::textGradientColor[1]);
        Vars::ESP::textGradientColor[2] = ParseFloat(json, "esp_textGradB", Vars::ESP::textGradientColor[2]);
        Vars::ESP::textGradientColor[3] = ParseFloat(json, "esp_textGradA", Vars::ESP::textGradientColor[3]);
        Vars::ESP::boxColor[0]    = ParseFloat(json, "esp_boxR",        Vars::ESP::boxColor[0]);
        Vars::ESP::boxColor[1]    = ParseFloat(json, "esp_boxG",        Vars::ESP::boxColor[1]);
        Vars::ESP::boxColor[2]    = ParseFloat(json, "esp_boxB",        Vars::ESP::boxColor[2]);
        Vars::ESP::boxColor[3]    = ParseFloat(json, "esp_boxA",        Vars::ESP::boxColor[3]);
        Vars::ESP::boxThickness   = ParseFloat(json, "esp_boxThick",    Vars::ESP::boxThickness);
        Vars::ESP::boxGradient    = ParseBool (json, "esp_boxGrad",     Vars::ESP::boxGradient);
        Vars::ESP::boxGradientColor[0] = ParseFloat(json, "esp_boxGradR", Vars::ESP::boxGradientColor[0]);
        Vars::ESP::boxGradientColor[1] = ParseFloat(json, "esp_boxGradG", Vars::ESP::boxGradientColor[1]);
        Vars::ESP::boxGradientColor[2] = ParseFloat(json, "esp_boxGradB", Vars::ESP::boxGradientColor[2]);
        Vars::ESP::boxGradientColor[3] = ParseFloat(json, "esp_boxGradA", Vars::ESP::boxGradientColor[3]);
        Vars::ESP::skeletonColor[0] = ParseFloat(json, "esp_skelR",     Vars::ESP::skeletonColor[0]);
        Vars::ESP::skeletonColor[1] = ParseFloat(json, "esp_skelG",     Vars::ESP::skeletonColor[1]);
        Vars::ESP::skeletonColor[2] = ParseFloat(json, "esp_skelB",     Vars::ESP::skeletonColor[2]);
        Vars::ESP::skeletonColor[3] = ParseFloat(json, "esp_skelA",     Vars::ESP::skeletonColor[3]);
        Vars::ESP::skeletonThickness = ParseFloat(json, "esp_skelThick", Vars::ESP::skeletonThickness);
        Vars::ESP::nameColor[0]   = ParseFloat(json, "esp_nameR",       Vars::ESP::nameColor[0]);
        Vars::ESP::nameColor[1]   = ParseFloat(json, "esp_nameG",       Vars::ESP::nameColor[1]);
        Vars::ESP::nameColor[2]   = ParseFloat(json, "esp_nameB",       Vars::ESP::nameColor[2]);
        Vars::ESP::nameColor[3]   = ParseFloat(json, "esp_nameA",       Vars::ESP::nameColor[3]);
        Vars::ESP::distanceColor[0] = ParseFloat(json, "esp_distR",     Vars::ESP::distanceColor[0]);
        Vars::ESP::distanceColor[1] = ParseFloat(json, "esp_distG",     Vars::ESP::distanceColor[1]);
        Vars::ESP::distanceColor[2] = ParseFloat(json, "esp_distB",     Vars::ESP::distanceColor[2]);
        Vars::ESP::distanceColor[3] = ParseFloat(json, "esp_distA",     Vars::ESP::distanceColor[3]);
        Vars::ESP::tracerColor[0] = ParseFloat(json, "esp_traceR",      Vars::ESP::tracerColor[0]);
        Vars::ESP::tracerColor[1] = ParseFloat(json, "esp_traceG",      Vars::ESP::tracerColor[1]);
        Vars::ESP::tracerColor[2] = ParseFloat(json, "esp_traceB",      Vars::ESP::tracerColor[2]);
        Vars::ESP::tracerColor[3] = ParseFloat(json, "esp_traceA",      Vars::ESP::tracerColor[3]);
        Vars::ESP::headDotColor[0] = ParseFloat(json, "esp_hDotR",      Vars::ESP::headDotColor[0]);
        Vars::ESP::headDotColor[1] = ParseFloat(json, "esp_hDotG",      Vars::ESP::headDotColor[1]);
        Vars::ESP::headDotColor[2] = ParseFloat(json, "esp_hDotB",      Vars::ESP::headDotColor[2]);
        Vars::ESP::headDotColor[3] = ParseFloat(json, "esp_hDotA",      Vars::ESP::headDotColor[3]);

        Vars::Local::speedEnabled       = ParseBool (json, "local_speedEnabled",  Vars::Local::speedEnabled);
        Vars::Local::walkSpeed          = ParseFloat(json, "local_walkSpeed",     Vars::Local::walkSpeed);
        Vars::Local::jumpEnabled        = ParseBool (json, "local_jumpEnabled",   Vars::Local::jumpEnabled);
        Vars::Local::jumpPower          = ParseFloat(json, "local_jumpPower",     Vars::Local::jumpPower);
        Vars::Local::flightEnabled      = ParseBool (json, "local_flightEnabled", Vars::Local::flightEnabled);
        Vars::Local::flightSpeed        = ParseFloat(json, "local_flightSpeed",   Vars::Local::flightSpeed);
        Vars::Local::flightKey          = ParseInt  (json, "local_flightKey",     Vars::Local::flightKey);
        Vars::Local::noclipEnabled      = ParseBool (json, "local_noclipEnabled", Vars::Local::noclipEnabled);
        Vars::Local::noclipKey          = ParseInt  (json, "local_noclipKey",     Vars::Local::noclipKey);
        Vars::Local::infiniteJumpEnabled= ParseBool (json, "local_infiniteJump",  Vars::Local::infiniteJumpEnabled);
        Vars::Local::desyncEnabled      = ParseBool (json, "local_desyncEnabled", Vars::Local::desyncEnabled);
        Vars::Local::desyncKey          = ParseInt  (json, "local_desyncKey",     Vars::Local::desyncKey);
        Vars::Local::desyncType         = ParseInt  (json, "local_desyncType",    Vars::Local::desyncType);
        Vars::Local::serverMode         = ParseInt  (json, "local_serverMode",    Vars::Local::serverMode);
        Vars::Local::clientMode         = ParseInt  (json, "local_clientMode",    Vars::Local::clientMode);

        Vars::Misc::streamProof         = ParseBool (json, "misc_streamProof",    Vars::Misc::streamProof);

        Vars::World::skybox             = ParseBool (json, "world_skybox",          Vars::World::skybox);
        Vars::World::skyboxType         = ParseInt  (json, "world_skyboxType",      Vars::World::skyboxType);
        Vars::World::rotateSkybox       = ParseBool (json, "world_rotateSkybox",    Vars::World::rotateSkybox);
        Vars::World::rotateSpeed        = ParseFloat(json, "world_rotateSpeed",     Vars::World::rotateSpeed);
        Vars::World::ambience           = ParseBool (json, "world_ambience",        Vars::World::ambience);
        Vars::World::fog                = ParseBool (json, "world_fog",             Vars::World::fog);
        Vars::World::fogDistance        = ParseFloat(json, "world_fogDistance",     Vars::World::fogDistance);
        Vars::World::brightness         = ParseBool (json, "world_brightness",      Vars::World::brightness);
        Vars::World::brightnessValue    = ParseFloat(json, "world_brightnessValue", Vars::World::brightnessValue);
        Vars::World::exposure           = ParseBool (json, "world_exposure",        Vars::World::exposure);
        Vars::World::exposureValue      = ParseFloat(json, "world_exposureValue",   Vars::World::exposureValue);
        Vars::World::fov                = ParseBool (json, "world_fov",             Vars::World::fov);
        Vars::World::fovValue           = ParseFloat(json, "world_fovValue",        Vars::World::fovValue);
    }

    // ── Public API ───────────────────────────────────────────────────────────

    static void EnsureDir() {
        CreateDirectoryA(CONFIG_DIR.c_str(), nullptr);
    }

    static bool Save(const std::string& name) {
        EnsureDir();
        std::string json     = BuildJson();
        std::string encoded  = Base64Encode(json);
        std::string path     = CONFIG_DIR + name + CONFIG_EXT;
        std::ofstream f(path, std::ios::out | std::ios::binary);
        if (!f.is_open()) return false;
        f << encoded;
        return true;
    }

    static bool Load(const std::string& name) {
        std::string path = CONFIG_DIR + name + CONFIG_EXT;
        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (!f.is_open()) return false;
        std::string encoded((std::istreambuf_iterator<char>(f)), {});
        std::string json = Base64Decode(encoded);
        ApplyJson(json);
        return true;
    }

    static std::vector<std::string> ListConfigs() {
        std::vector<std::string> names;
        WIN32_FIND_DATAA fd;
        std::string pattern = CONFIG_DIR + "*" + CONFIG_EXT;
        HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) return names;
        do {
            std::string fname = fd.cFileName;
            auto dot = fname.rfind('.');
            if (dot != std::string::npos) fname = fname.substr(0, dot);
            names.push_back(fname);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        return names;
    }
}
