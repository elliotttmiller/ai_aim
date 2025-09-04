// src/Overlay/Memory/GameData.h
#pragma once
#include "../../IPC/SharedStructs.h"
#include "../Utils/Singleton.h"
#ifdef _WIN32
    #include <Windows.h>
#else
    // Cross-platform stubs
    typedef void* HANDLE;
    typedef unsigned long DWORD;
    typedef unsigned char BYTE;
    typedef size_t SIZE_T;
#endif
#include <vector>

// --- Data Structures ---
// CRITICAL: These structs MUST perfectly match the memory layout in AimTrainer's main.cpp
// struct Vec3 { float x, y, z; };
//
// struct RaylibCamera {
//     Vec3 position;
//     Vec3 target;
//     Vec3 up;
//     float fovy;
//     int projection;
// };
//
// struct RaylibTarget {
//     Vec3 position;
//     bool active;
//     float lifeTimer;
// };

class GameData : public Singleton<GameData> {
public:
    void Scan();
    void Update();
    void UpdateFromIPC();
    void SendToIPC();
    uintptr_t GetCameraAddress() const;
    const RaylibCamera& GetCamera() const { return m_Camera; }
    const std::vector<RaylibTarget>& GetTargets() const { return m_Targets; }
    bool IsReady() const { return m_bReady; }

private:
    bool m_bReady = false;
    uintptr_t m_pCamera = 0;
    uintptr_t m_pTargetsVec = 0;
    RaylibCamera m_Camera;
    std::vector<RaylibTarget> m_Targets;

    // Config and validation helpers
    const char* GetConfigModuleName() const;
    const char* GetConfigSignaturePattern() const;
    bool ValidateCamera(const RaylibCamera* cam) const;
    bool ValidateTargetsVector(const std::vector<RaylibTarget>* tgtVec) const;
};
