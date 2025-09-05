#include "../Utils/UnifiedAimAssist.h"
#include "../Utils/Logger.h"

/**
 * Unified Overlay Main
 * 
 * Simplified overlay that uses the unified aim assist system
 * and integrates with the memory injection workflow.
 */

class UnifiedOverlay {
public:
    static UnifiedOverlay& GetInstance() {
        static UnifiedOverlay instance;
        return instance;
    }
    
    bool Initialize() {
        Logger::Get().Log("UnifiedOverlay", "Initializing unified overlay system...");
        
        // Initialize the unified aim assist
        auto& aimAssist = UnifiedAimAssist::GetInstance();
        if (!aimAssist.Initialize()) {
            Logger::Get().Log("UnifiedOverlay", "Failed to initialize aim assist");
            return false;
        }
        
        m_initialized = true;
        Logger::Get().Log("UnifiedOverlay", "Unified overlay initialized successfully");
        return true;
    }
    
    void Update() {
        if (!m_initialized) return;
        
        // Update the unified aim assist system
        auto& aimAssist = UnifiedAimAssist::GetInstance();
        aimAssist.Update();
    }
    
    void Shutdown() {
        if (m_initialized) {
            auto& aimAssist = UnifiedAimAssist::GetInstance();
            aimAssist.Shutdown();
            m_initialized = false;
            Logger::Get().Log("UnifiedOverlay", "Unified overlay shut down");
        }
    }
    
    bool IsInitialized() const { return m_initialized; }

private:
    UnifiedOverlay() = default;
    ~UnifiedOverlay() { Shutdown(); }
    
    bool m_initialized = false;
};

// Export functions for injection DLL
extern "C" {
    bool InitializeOverlay() {
        return UnifiedOverlay::GetInstance().Initialize();
    }
    
    void UpdateOverlay() {
        UnifiedOverlay::GetInstance().Update();
    }
    
    void ShutdownOverlay() {
        UnifiedOverlay::GetInstance().Shutdown();
    }
}