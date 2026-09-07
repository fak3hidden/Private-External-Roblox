#pragma once
#include <string>

namespace Vars {
    // ==========================================
    // Global UI & Menu States
    // ==========================================
    inline bool menuOpen = false;
    inline int  selectedTab = 0;
    inline bool unlockCursor = false;

    namespace Explorer {
        inline bool enabled = false;
    }

    // ==========================================
    // Aimbot
    // ==========================================
    namespace Aimbot {
        // General
        inline bool  enabled = false;
        inline bool  showFOV = false;
        inline float fovRadius = 100.0f;
        inline float smoothing = 5.0f;
        inline int   aimTarget = 0;
        inline int   aimMethod = 0;
        inline int   aimbotKey = 2;
        inline bool  enableCameraAimbot = false;

        // Checks
        inline bool  teamCheck = false;
        inline bool  downedCheck = true;
        inline bool  invisCheck = false;
        inline bool  invincibleCheck = false;

        // Prediction
        inline bool  prediction = false;
        inline float predX = 0.0f;
        inline float predY = 0.0f;

        // Silent Aim
        inline bool  silentAim = false;
        inline bool  showSilentFOV = false;
        inline float silentFOV = 100.0f;
        inline int   silentAimbotKey = 2;
        inline bool  silentPrediction = false;
        inline bool  randomTarget = false;
    }

    // ==========================================
    // Triggerbot
    // ==========================================
    namespace Triggerbot {
        inline bool  enabled = false;
        inline int   key = 2;
        inline bool  hitboxes[4] = { true, true, false, false };
        inline float hitbox_scale = 1.0f;
        inline float delay = 0.0f;
        inline float interval = 80.0f;
        inline float fov = 40.0f;
        inline bool  teamCheck = false;
    }

    // ==========================================
    // ESP (Extra Sensory Perception)
    // ==========================================
    namespace ESP {
        // Main Toggles
        inline bool  enabled = false;
        inline bool  teamCheck = false;
        inline bool  offscreen = false;

        // Box Settings
        inline int   boxMode = 1;
        inline bool  filledBox = false;
        inline bool  staticBox = false;
        inline float boxThickness = 1.0f;
        inline bool  boxGradient = false;

        // Text Settings
        inline int   nameMode = 1;
        inline bool  distance = false;
        inline bool  healthBar = false;
        inline bool  healthText = false;
        inline bool  textGradient = false;

        // Effects
        inline bool  tracers = false;
        inline int   tracerOrigin = 2;
        inline bool  skeleton = false;
        inline bool  headDot = false;
        inline bool  viewAngle = false;

        // Colors (RGBA Float 0.0 - 1.0)
        inline float filledColor[4] = { 1.0f, 0.2f, 0.2f, 0.25f };
        inline float boxColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline float boxGradientColor[4] = { 0.45f, 0.30f, 0.85f, 1.0f };
        inline float textGradientColor[4] = { 0.45f, 0.30f, 0.85f, 1.0f };
        inline float nameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline float distanceColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline float tracerColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline float headDotColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Skeleton
        inline float skeletonThickness = 1.5f;
        inline float skeletonColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };

        // Crosshair
        inline bool  crosshair = false;
        inline float crosshairSize = 10.0f;
        inline float crosshairThickness = 1.0f;
        inline float crosshairGap = 2.0f;
        inline bool  crosshairDot = false;
        inline float crosshairColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    // ==========================================
    // Local Player
    // ==========================================
    namespace Local {
        // Movement
        inline bool  speedEnabled = false;
        inline float walkSpeed = 16.0f;
        inline bool  jumpEnabled = false;
        inline float jumpPower = 50.0f;
        inline bool  infiniteJumpEnabled = false;

        // Flight
        inline bool  flightEnabled = false;
        inline float flightSpeed = 50.0f;
        inline int   flightKey = 0;

        // Noclip
        inline bool  noclipEnabled = false;
        inline int   noclipKey = 0;

        // Desync
        inline bool  desyncEnabled = false;
        inline int   desyncKey = 0;
        inline int   desyncType = 0;
        inline int   serverMode = 0;
        inline int   clientMode = 0;
    }

    // ==========================================
    // World & Environment
    // ==========================================
    namespace World {
        // Skybox
        inline bool  skybox = false;
        inline int   skyboxType = 0;
        inline bool  rotateSkybox = false;
        inline float rotateSpeed = 0.5f;

        // Lighting
        inline bool  ambience = false;
        inline float ambienceColor[3] = { 1.0f, 1.0f, 1.0f };
        inline bool  brightness = false;
        inline float brightnessValue = 1.0f;
        inline bool  exposure = false;
        inline float exposureValue = 1.0f;

        // Fog
        inline bool  fog = false;
        inline float fogDistance = 1000.0f;
        inline float fogColor[3] = { 1.0f, 1.0f, 1.0f };

        // Camera
        inline bool  fov = false;
        inline float fovValue = 70.0f;
    }

    // ==========================================
    // Rage
    // ==========================================
    namespace Rage {
        inline bool  antiAim = false;
        inline int   pitch = 0;
        inline int   yaw = 0;
    }

    // ==========================================
    // Miscellaneous
    // ==========================================
    namespace Misc {
        inline bool  streamProof = false;
        inline bool  fastLaunch = false;
        inline bool  rescan = false;
        inline bool  autoRescan = true;
    }
}