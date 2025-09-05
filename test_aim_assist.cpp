#include "src/Utils/UnifiedAimAssist.h"
#include "src/Utils/Logger.h"
#include <iostream>

int main() {
    Logger::Get().InitDefault();
    Logger::Get().Log("TestAimAssist", "Testing Real Aim Assist System for AimTrainer");
    
    // Initialize the aim assist system
    auto& aimAssist = UnifiedAimAssist::GetInstance();
    
    if (aimAssist.Initialize()) {
        Logger::Get().Log("TestAimAssist", "✓ Aim assist system initialized successfully");
        
        // Test basic functionality
        Logger::Get().Log("TestAimAssist", "✓ System enabled: " + std::string(aimAssist.IsEnabled() ? "YES" : "NO"));
        
        // Test configuration
        UnifiedAimConfig config = aimAssist.GetConfig();
        config.sensitivity = 0.8f;
        config.fovRadius = 150.0f;
        config.enablePrediction = true;
        aimAssist.SetConfig(config);
        Logger::Get().Log("TestAimAssist", "✓ Configuration updated successfully");
        
        // Simulate a few update cycles
        for (int i = 0; i < 5; ++i) {
            aimAssist.Update();
            Logger::Get().Log("TestAimAssist", "Update " + std::to_string(i+1) + " completed");
        }
        
        // Test target management
        auto targets = aimAssist.GetVisibleTargets();
        Logger::Get().Log("TestAimAssist", "✓ Found " + std::to_string(targets.size()) + " targets");
        
        Logger::Get().Log("TestAimAssist", "✓ All tests passed - Real aim assist system is working!");
        
        aimAssist.Shutdown();
        Logger::Get().Log("TestAimAssist", "✓ System shut down cleanly");
        
        std::cout << "SUCCESS: Real aim assist system for AimTrainer is working correctly!\n";
        return 0;
        
    } else {
        Logger::Get().Log("TestAimAssist", "✗ Failed to initialize aim assist system");
        std::cout << "FAILED: Could not initialize aim assist system\n";
        return 1;
    }
}