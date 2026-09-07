#pragma once
#include "../Offsets/Offsets.h"
#include "../../Memory/Communication.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono> // ADDED TO FIX CHRONO ERRORS

namespace RBX {

    // ==========================================
    // Math Structures
    // ==========================================
    struct Vec2 {
        float X{ 0.0f };
        float Y{ 0.0f };
    };

    struct Vec3 {
        float X{ 0.0f };
        float Y{ 0.0f };
        float Z{ 0.0f };

        Vec3 operator+(const Vec3& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
        Vec3 operator-(const Vec3& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }
        Vec3 operator*(float scalar) const { return { X * scalar, Y * scalar, Z * scalar }; }

        float Dot(const Vec3& other) const { return (X * other.X) + (Y * other.Y) + (Z * other.Z); }
        float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
        float Distance(const Vec3& other) const { return (*this - other).Length(); }
    };

    struct Vec4 {
        float X{ 0.0f };
        float Y{ 0.0f };
        float Z{ 0.0f };
        float W{ 0.0f };
    };

    struct Mat3 {
        float data[9]{ 0 };
    };

    struct Mat4 {
        float data[16]{ 0 };
    };

    struct Rotation {
        float data[12]{ 0 };

        Vec3 GetRightVector() const { return { data[0], data[3], data[6] }; }
        Vec3 GetUpVector() const { return { data[1], data[4], data[7] }; }
        Vec3 GetLookVector() const { return { data[2], data[5], data[8] }; }
        Vec3 GetPosition() const { return { data[9], data[10], data[11] }; }
    };

    // ==========================================
    // Instance Wrapper
    // ==========================================
    class RbxInstance {
    public:
        uintptr_t Addr{ 0 };

        RbxInstance() = default;
        RbxInstance(uintptr_t addr) : Addr(addr) {}

        bool IsValid() const { return Addr != 0; }

        std::string GetName() const {
            if (!IsValid()) return "";
            uintptr_t namePtr = Coms->ReadMemory<uintptr_t>(Addr + Offsets::Instance::Name);
            if (namePtr == 0) return "";
            return Coms->ReadGameString(namePtr);
        }

        std::string GetClass() const {
            if (!IsValid()) return "";
            uintptr_t classDesc = Coms->ReadMemory<uintptr_t>(Addr + Offsets::Instance::ClassDescriptor);
            if (classDesc == 0) return "";
            uintptr_t namePtr = Coms->ReadMemory<uintptr_t>(classDesc + Offsets::Instance::ClassName);
            if (namePtr == 0) return "";
            return Coms->ReadGameString(namePtr);
        }

        RbxInstance GetParent() const {
            if (!IsValid()) return RbxInstance(0);
            return RbxInstance(Coms->ReadMemory<uintptr_t>(Addr + Offsets::Instance::Parent));
        }

        std::vector<RbxInstance> GetChildList() const {
            std::vector<RbxInstance> childList;
            if (!IsValid()) return childList;

            uintptr_t childStart = Coms->ReadMemory<uintptr_t>(Addr + Offsets::Instance::ChildrenStart);
            if (childStart == 0) return childList;

            uintptr_t childEnd = Coms->ReadMemory<uintptr_t>(childStart + Offsets::Instance::ChildrenEnd);
            uintptr_t current = Coms->ReadMemory<uintptr_t>(childStart);

            if (current == 0 || childEnd <= current) return childList;

            size_t count = (childEnd - current) / 0x10;
            if (count > 10000) count = 10000; // Cap to prevent crash

            childList.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                uintptr_t childAddr = Coms->ReadMemory<uintptr_t>(current + i * 0x10);
                if (childAddr != 0) childList.emplace_back(childAddr);
            }
            return childList;
        }

        RbxInstance FindChild(const std::string& targetName) const {
            for (const auto& child : GetChildList()) {
                if (child.GetName() == targetName) return child;
            }
            return RbxInstance(0);
        }

        RbxInstance FindChildByClass(const std::string& targetClass) const {
            for (const auto& child : GetChildList()) {
                if (child.GetClass() == targetClass) return child;
            }
            return RbxInstance(0);
        }

        RbxInstance WaitChild(const std::string& targetName, int timeoutMs = 5000) const {
            auto start = std::chrono::high_resolution_clock::now();
            while (true) {
                auto child = FindChild(targetName);
                if (child.IsValid()) return child;

                if (timeoutMs > 0) {
                    auto now = std::chrono::high_resolution_clock::now();
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeoutMs) {
                        return RbxInstance(0); // Timeout
                    }
                }
                Sleep(50);
            }
        }

        RbxInstance FindCharacterPart(const std::string& targetName) const {
            auto part = FindChild(targetName);
            if (part.IsValid()) return part;

            auto hitbox = FindChild("Hitbox");
            if (hitbox.IsValid()) {
                part = hitbox.FindChild(targetName);
                if (part.IsValid()) return part;
            }

            const RbxInstance& searchFolder = hitbox.IsValid() ? hitbox : *this;

            if (targetName == "Torso" || targetName == "UpperTorso") {
                part = searchFolder.FindChild("Chest");     if (part.IsValid()) return part;
                part = searchFolder.FindChild("Abdomen");   if (part.IsValid()) return part;
                part = searchFolder.FindChild("Hips");      if (part.IsValid()) return part;
            }
            else if (targetName == "LowerTorso") {
                part = searchFolder.FindChild("Abdomen");   if (part.IsValid()) return part;
                part = searchFolder.FindChild("Hips");      if (part.IsValid()) return part;
            }
            else if (targetName == "HumanoidRootPart") {
                part = searchFolder.FindChild("Root");      if (part.IsValid()) return part;
            }
            else if (targetName.find("Left") != std::string::npos && targetName.find("Arm") != std::string::npos) {
                part = searchFolder.FindChild("LeftArm");   if (part.IsValid()) return part;
            }
            else if (targetName.find("Right") != std::string::npos && targetName.find("Arm") != std::string::npos) {
                part = searchFolder.FindChild("RightArm");  if (part.IsValid()) return part;
            }
            else if (targetName.find("Left") != std::string::npos && targetName.find("Leg") != std::string::npos) {
                part = searchFolder.FindChild("LeftLeg");   if (part.IsValid()) return part;
            }
            else if (targetName.find("Right") != std::string::npos && targetName.find("Leg") != std::string::npos) {
                part = searchFolder.FindChild("RightLeg");  if (part.IsValid()) return part;
            }

            return RbxInstance(0);
        }

        uintptr_t GetPrimitivePtr() const {
            if (!IsValid()) return 0;
            return Coms->ReadMemory<uintptr_t>(Addr + Offsets::BasePart::Primitive);
        }

        Vec3 GetPos() const {
            uintptr_t prim = GetPrimitivePtr();
            if (!prim) return { 0, 0, 0 };
            return Coms->ReadMemory<Vec3>(prim + Offsets::Primitive::Position);
        }

        Rotation GetRotation() const {
            uintptr_t prim = GetPrimitivePtr();
            if (!prim) return Rotation{};
            return Coms->ReadMemory<Rotation>(prim + Offsets::Primitive::Rotation);
        }

        // Camera specifics
        Mat3 GetCameraRotation() const {
            if (!IsValid()) return Mat3{};
            return Coms->ReadMemory<Mat3>(Addr + Offsets::Camera::Rotation);
        }

        Vec3 GetCameraPosition() const {
            if (!IsValid()) return { 0, 0, 0 };
            return Coms->ReadMemory<Vec3>(Addr + Offsets::Camera::Position);
        }

        void SetCameraRotation(const Mat3& rot) const {
            if (!IsValid()) return;
            Coms->WriteMemory(Addr + Offsets::Camera::Rotation, rot);
        }

        // Resolve this Player's character model. Robust to a stale ModelInstance
        // offset: validates the pointer, then falls back to name lookup since
        // character models are (almost) always named after the player.
        RbxInstance GetModelRef() const {
            if (!IsValid()) return RbxInstance(0);

            // Fast path: direct pointer, validated (characters are Models).
            uintptr_t viaPtr = Coms->ReadMemory<uintptr_t>(Addr + Offsets::Player::ModelInstance);
            if (viaPtr != 0) {
                RbxInstance m(viaPtr);
                if (m.GetClass() == "Model") return m;
            }

            // Fallback: Workspace/Characters/<Name> (Bad Business style), then Workspace/<Name>.
            std::string name = GetName();
            if (!name.empty()) {
                RbxInstance parent = GetParent();
                if (parent.IsValid()) {
                    RbxInstance dataModel = parent.GetParent();
                    if (dataModel.IsValid()) {
                        RbxInstance workspace = dataModel.FindChildByClass("Workspace");
                        if (workspace.IsValid()) {
                            RbxInstance charactersFolder = workspace.FindChild("Characters");
                            if (charactersFolder.IsValid()) {
                                RbxInstance charModel = charactersFolder.FindChild(name);
                                if (charModel.IsValid()) return charModel;
                            }
                            RbxInstance charModel = workspace.FindChild(name);
                            if (charModel.IsValid()) return charModel;
                        }
                    }
                }
            }
            return RbxInstance(0);
        }

        float CalcDistance(const Vec3& targetPos) const {
            return GetPos().Distance(targetPos);
        }

        bool IsInvincible() const {
            if (!IsValid()) return false;
            for (const auto& child : GetChildList()) {
                if (child.GetClass() == "ForceField") return true;
            }
            return false;
        }

        bool IsTransparent() const {
            auto head = FindCharacterPart("Head");
            if (!head.IsValid()) return false;
            float transparency = Coms->ReadMemory<float>(head.Addr + Offsets::BasePart::Transparency);
            return (transparency > 0.01f);
        }
    };

    // ==========================================
    // Render Engine
    // ==========================================
    class RenderEngine : public RbxInstance {
    public:
        RenderEngine() = default;
        RenderEngine(uintptr_t addr) : RbxInstance(addr) {}

        Mat4 GetViewMat() const {
            if (!IsValid()) return Mat4{};
            return Coms->ReadMemory<Mat4>(Addr + Offsets::VisualEngine::ViewMatrix);
        }

        Vec2 WorldToViewport(const Vec3& worldPos) const {
            Vec4 quat;
            Vec2 screenDims{ static_cast<float>(GetSystemMetrics(SM_CXSCREEN)),
                             static_cast<float>(GetSystemMetrics(SM_CYSCREEN)) };

            Mat4 viewMat = GetViewMat();

            quat.X = (worldPos.X * viewMat.data[0]) + (worldPos.Y * viewMat.data[1]) + (worldPos.Z * viewMat.data[2]) + viewMat.data[3];
            quat.Y = (worldPos.X * viewMat.data[4]) + (worldPos.Y * viewMat.data[5]) + (worldPos.Z * viewMat.data[6]) + viewMat.data[7];
            quat.Z = (worldPos.X * viewMat.data[8]) + (worldPos.Y * viewMat.data[9]) + (worldPos.Z * viewMat.data[10]) + viewMat.data[11];
            quat.W = (worldPos.X * viewMat.data[12]) + (worldPos.Y * viewMat.data[13]) + (worldPos.Z * viewMat.data[14]) + viewMat.data[15];

            if (quat.W < 0.1f) return { 0.0f, 0.0f }; // Behind camera

            Vec3 ndc;
            ndc.X = quat.X / quat.W;
            ndc.Y = quat.Y / quat.W;

            Vec2 screenPos;
            screenPos.X = (screenDims.X / 2.0f * ndc.X) + (screenDims.X / 2.0f);
            screenPos.Y = -(screenDims.Y / 2.0f * ndc.Y) + (screenDims.Y / 2.0f);

            return screenPos;
        }
    };

    // ==========================================
    // Global Helper Functions
    // ==========================================
    inline void ModifyWalkspeed(const RbxInstance& humanoid, float newSpeed) {
        if (humanoid.IsValid()) Coms->WriteMemory(humanoid.Addr + Offsets::Humanoid::Walkspeed, newSpeed);
    }

    inline void ModifyJumpPower(const RbxInstance& humanoid, float newPower) {
        if (humanoid.IsValid()) Coms->WriteMemory(humanoid.Addr + Offsets::Humanoid::JumpPower, newPower);
    }

    inline float GetNumberValue(const RbxInstance& valObj) {
        if (!valObj.IsValid()) return 0.0f;
        std::string className = valObj.GetClass();

        if (className == "NumberValue" || className == "DoubleValue") {
            return static_cast<float>(Coms->ReadMemory<double>(valObj.Addr + Offsets::Misc::Value));
        }
        else if (className == "IntValue") {
            return static_cast<float>(Coms->ReadMemory<int64_t>(valObj.Addr + Offsets::Misc::Value));
        }
        return 0.0f;
    }
}