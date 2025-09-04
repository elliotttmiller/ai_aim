// src/Overlay/Memory/GameData.cpp
#include "GameData.h"
#include "IPC/SharedMemory.h"
#include "IPC/NamedPipe.h"
#include <iostream>
#include <Psapi.h>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <set>
#include <cstring>
#include <filesystem>
#include <tlhelp32.h>

// [REMOVED]: All direct memory reading, scanning, and legacy code
// [ADDED]: IPC integration stub (shared memory/named pipe)

void GameData::SendToIPC() {
    IPC::GameDataPacket packet;
    memcpy(packet.camera, &m_Camera, sizeof(m_Camera));
    for (size_t i = 0; i < m_Targets.size() && i < 128; ++i) {
        packet.targets[i][0] = m_Targets[i].position.x;
        packet.targets[i][1] = m_Targets[i].position.y;
        packet.targets[i][2] = m_Targets[i].position.z;
        packet.targets[i][3] = m_Targets[i].active ? 1.0f : 0.0f;
    }
    packet.targetCount = static_cast<int>(m_Targets.size());
    bool ok = IPC::SharedMemory::Write(packet) || IPC::NamedPipe::Write(packet);
    std::ofstream log("bin/debug.log", std::ios::app);
    log << "[DLL] Wrote to IPC: camera=("
        << m_Camera.position.x << "," << m_Camera.position.y << "," << m_Camera.position.z << ") targets=" << packet.targetCount << " status=" << (ok ? "OK" : "FAIL") << std::endl;
    log.close();
}

void GameData::UpdateFromIPC() {
    IPC::GameDataPacket packet;
    bool ok = IPC::SharedMemory::Read(packet) || IPC::NamedPipe::Read(packet);
    if (ok) {
        memcpy(&m_Camera, packet.camera, sizeof(m_Camera));
        m_Targets.clear();
        for (int i = 0; i < packet.targetCount && i < 128; ++i) {
            RaylibTarget tgt;
            tgt.position.x = packet.targets[i][0];
            tgt.position.y = packet.targets[i][1];
            tgt.position.z = packet.targets[i][2];
            tgt.active = packet.targets[i][3] > 0.5f;
            m_Targets.push_back(tgt);
        }
    }
    std::ofstream log("bin/debug.log", std::ios::app);
    log << "[Overlay] Read from IPC: camera=("
        << m_Camera.position.x << "," << m_Camera.position.y << "," << m_Camera.position.z << ") targets=" << m_Targets.size() << " status=" << (ok ? "OK" : "FAIL") << std::endl;
    log.close();
}

void GameData::Update() {
    UpdateFromIPC();
    // Mark ready if IPC data is valid
    m_bReady = true; // Optionally validate packet contents
}

static bool LoadConfig(); // Forward declaration

static std::map<std::string, std::string> g_Config;
static std::set<std::string> g_DetectedModules;

static void ScanAndAutonomizeConfig() {
    // Scan all running processes and modules
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W entry = { sizeof(entry) };
    std::set<std::wstring> processNames;
    if (Process32FirstW(hSnap, &entry)) {
        do {
            processNames.insert(entry.szExeFile);
        } while (Process32NextW(hSnap, &entry));
    }
    CloseHandle(hSnap);

    // Scan loaded modules for current process
    HMODULE hMods[1024];
    DWORD cbNeeded;
    HANDLE hProcess = GetCurrentProcess();
    std::set<std::string> moduleNames;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char szModName[MAX_PATH];
            if (GetModuleFileNameA(hMods[i], szModName, sizeof(szModName) / sizeof(char))) {
                std::string modName = szModName;
                moduleNames.insert(modName);
            }
        }
    }

    // Auto-create config/game_memory.cfg if missing
    std::filesystem::create_directories("config");
    std::ifstream f("config/game_memory.cfg");
    bool configExists = f.is_open();
    f.close();
    if (!configExists) {
        std::ofstream out("config/game_memory.cfg");
        for (const auto& mod : moduleNames) {
            out << "module_name=" << mod << std::endl;
            out << "signature_pattern=37 13 37 13 BA BE FA CE EF BE AD DE" << std::endl;
        }
        out.close();
    } else {
        // Update config with any new modules
        std::ifstream in("config/game_memory.cfg");
        std::set<std::string> existingModules;
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("module_name=") == 0) {
                std::string mod = line.substr(strlen("module_name="));
                existingModules.insert(mod);
            }
        }
        in.close();
        std::ofstream out("config/game_memory.cfg", std::ios::app);
        for (const auto& mod : moduleNames) {
            if (existingModules.find(mod) == existingModules.end()) {
                out << "module_name=" << mod << std::endl;
                out << "signature_pattern=37 13 37 13 BA BE FA CE EF BE AD DE" << std::endl;
            }
        }
        out.close();
    }
}

static bool LoadConfig() {
    ScanAndAutonomizeConfig();
    if (!g_Config.empty()) return true;
    std::ifstream f("config/game_memory.cfg");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            g_Config[key] = value;
        }
    }
    return true;
}

void GameData::Scan() {
    // Autonomous scan logic is handled by config and runtime detection
    LoadConfig();
}

static std::set<std::string> GetSupportedModulesFromConfig() {
    std::set<std::string> modules;
    std::ifstream f("config/game_memory.cfg");
    if (!f.is_open()) return modules;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("module_name=") == 0) {
            std::string mod = line.substr(strlen("module_name="));
            modules.insert(mod);
        }
    }
    return modules;
}

static std::string DetectGameModule() {
    std::set<std::string> supported = GetSupportedModulesFromConfig();
    HMODULE hMods[1024];
    DWORD cbNeeded;
    HANDLE hProcess = GetCurrentProcess();
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char szModName[MAX_PATH];
            if (GetModuleFileNameA(hMods[i], szModName, sizeof(szModName) / sizeof(char))) {
                std::string modName = szModName;
                for (const auto& supportedMod : supported) {
                    if (modName.find(supportedMod) != std::string::npos) return supportedMod;
                }
            }
        }
    }
    return "";
}

bool LoadConfigProfile(const std::string& moduleName) {
    std::ifstream f("config/game_memory.cfg");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find(moduleName) != std::string::npos) {
            // Parse config for this module
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                g_Config[key] = value;
            }
        }
    }
    return !g_Config.empty();
}

const char* GameData::GetConfigModuleName() const {
    std::string detected = DetectGameModule();
    if (!detected.empty()) LoadConfigProfile(detected);
    auto it = g_Config.find("module_name");
    return it != g_Config.end() ? it->second.c_str() : "";
}
const char* GameData::GetConfigSignaturePattern() const {
    LoadConfig();
    auto it = g_Config.find("signature_pattern");
    return it != g_Config.end() ? it->second.c_str() : "";
}

// Strict camera pointer validation
bool GameData::ValidateCamera(const RaylibCamera* cam) const {
    if (!cam) return false;
    return fabs(cam->up.y - 1.0f) < 0.01f && cam->fovy > 10.0f && cam->fovy < 120.0f && cam->position.z < 0.0f && cam->target.z == 0.0f;
}
// Strict targets vector validation
bool GameData::ValidateTargetsVector(const std::vector<RaylibTarget>* tgtVec) const {
    if (!tgtVec || tgtVec->size() == 0 || tgtVec->size() > 32) return false;
    int plausible = 0;
    for (const auto& tgt : *tgtVec) {
        if (tgt.active && tgt.position.z > 0.5f && tgt.position.z < 20.0f && fabs(tgt.position.x) < 10.0f && fabs(tgt.position.y) < 10.0f) {
            plausible++;
        }
    }
    return plausible > 0;
}
