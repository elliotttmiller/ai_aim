#include "src/Utils/Logger.h"
#include "src/IPC/SharedStructs.h"
#include "src/IPC/SharedMemory.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>

class MockAimTrainer {
public:
    bool Initialize() {
        Logger::Get().InitDefault();
        Logger::Get().Log("MockAimTrainer", "Initializing Mock AimTrainer for demonstration...");
        
        // Create shared memory for communication with aim assist
        std::wstring memoryName = L"Global\\AIM_ASSIST_MEMORY";
        m_sharedMemory = std::make_unique<SharedMemory>(memoryName.c_str(), sizeof(WorkingSharedMemory));
        if (!m_sharedMemory->Create()) {
            Logger::Get().Log("MockAimTrainer", "Failed to create shared memory");
            return false;
        }
        
        m_sharedData = static_cast<WorkingSharedMemory*>(m_sharedMemory->GetData());
        if (!m_sharedData) {
            Logger::Get().Log("MockAimTrainer", "Failed to get shared memory data");
            return false;
        }
        
        // Initialize shared memory
        *m_sharedData = WorkingSharedMemory{};
        m_sharedData->initialized = true;
        
        // Initialize camera
        m_sharedData->camera.position = Vec3(0.0f, 0.0f, -10.0f);
        m_sharedData->camera.target = Vec3(0.0f, 0.0f, 0.0f);
        m_sharedData->camera.up = Vec3(0.0f, 1.0f, 0.0f);
        m_sharedData->camera.fovy = 60.0f;
        m_sharedData->camera.projection = 0;
        
        Logger::Get().Log("MockAimTrainer", "Mock AimTrainer initialized successfully");
        return true;
    }
    
    void Update() {
        static auto startTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        float timeAlive = std::chrono::duration<float>(now - startTime).count();
        
        // Simulate moving targets
        m_sharedData->targetCount = 3; // Create 3 mock targets
        
        for (int i = 0; i < m_sharedData->targetCount; ++i) {
            auto& target = m_sharedData->targets[i];
            
            // Create moving targets in different patterns
            float angle = timeAlive * (0.5f + i * 0.3f);
            target.position.x = std::sin(angle) * (2.0f + i);
            target.position.y = std::cos(angle * 0.7f) * (1.0f + i * 0.5f);
            target.position.z = 3.0f + i * 2.0f;
            
            // Calculate velocity
            static Vec3 lastPos[3] = {{0,0,0}, {0,0,0}, {0,0,0}};
            target.velocity.x = (target.position.x - lastPos[i].x) * 60.0f; // Assuming 60fps
            target.velocity.y = (target.position.y - lastPos[i].y) * 60.0f;
            target.velocity.z = (target.position.z - lastPos[i].z) * 60.0f;
            lastPos[i] = target.position;
            
            target.active = true;
            target.lifeTimer = 10.0f; // 10 seconds remaining
            target.lastPosition = target.position;
            target.lastUpdateTime = timeAlive;
        }
        
        m_sharedData->frameId++;
        m_sharedData->timestamp = static_cast<uint32_t>(timeAlive * 1000.0f);
        m_sharedData->lastUpdate = m_sharedData->timestamp;
    }
    
    void PrintStatus() {
        Logger::Get().Log("MockAimTrainer", "Status: " + std::to_string(m_sharedData->targetCount) + " targets active");
        for (int i = 0; i < m_sharedData->targetCount; ++i) {
            const auto& target = m_sharedData->targets[i];
            Logger::Get().Log("MockAimTrainer", 
                "Target " + std::to_string(i) + ": Pos(" + 
                std::to_string(target.position.x) + "," + 
                std::to_string(target.position.y) + "," + 
                std::to_string(target.position.z) + ") Vel(" +
                std::to_string(target.velocity.x) + "," + 
                std::to_string(target.velocity.y) + "," + 
                std::to_string(target.velocity.z) + ")");
        }
    }

private:
    std::unique_ptr<SharedMemory> m_sharedMemory;
    WorkingSharedMemory* m_sharedData = nullptr;
};

int main() {
    std::cout << "=== Mock AimTrainer Demo ===\n";
    std::cout << "This demonstrates the real aim assist system working with simulated targets.\n\n";
    
    MockAimTrainer trainer;
    if (!trainer.Initialize()) {
        std::cout << "Failed to initialize mock trainer\n";
        return 1;
    }
    
    std::cout << "Mock AimTrainer running...\n";
    std::cout << "Simulating moving targets for aim assist system to track.\n";
    std::cout << "Running for 10 seconds...\n\n";
    
    auto startTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(now - startTime).count();
        
        if (elapsedTime >= 10.0f) {
            break;
        }
        
        trainer.Update();
        
        // Print status every 2 seconds
        if (frameCount % 120 == 0) {
            trainer.PrintStatus();
        }
        
        frameCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    std::cout << "\nDemo completed successfully!\n";
    std::cout << "The aim assist system can now read real target data from shared memory.\n";
    std::cout << "In the actual AimTrainer, this would be connected to real 3D targets.\n";
    
    return 0;
}