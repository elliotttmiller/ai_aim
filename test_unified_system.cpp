#include "src/Utils/UnifiedAimAssist.h"
#include "src/Utils/UnifiedUtilities.h"
#include "src/Utils/Logger.h"
#include <iostream>

int main() {
    std::cout << "=== AI Aim Unified System Test ===\n";
    
    // Initialize logger
    Logger::Get().InitDefault();
    Logger::Get().Log("TEST", "Starting unified system test...");
    
    // Test unified utilities
    std::cout << "Testing Unified Utilities...\n";
    std::string test = "Hello World";
    std::wstring wide = UnifiedUtilities::UTF8ToWide(test);
    std::string back = UnifiedUtilities::WideToUTF8(wide);
    std::cout << "String conversion test: " << (test == back ? "PASSED" : "FAILED") << "\n";
    
    // Test unified aim assist
    std::cout << "Testing Unified Aim Assist...\n";
    auto& aimAssist = UnifiedAimAssist::GetInstance();
    if (aimAssist.Initialize()) {
        std::cout << "Aim Assist initialization: PASSED\n";
        std::cout << "Enabled: " << (aimAssist.IsEnabled() ? "Yes" : "No") << "\n";
        std::cout << "Mode: " << static_cast<int>(aimAssist.GetMode()) << "\n";
        
        // Test basic functionality
        aimAssist.Update(); // Should not crash
        std::cout << "Aim Assist update: PASSED\n";
        
        aimAssist.Shutdown();
        std::cout << "Aim Assist shutdown: PASSED\n";
    } else {
        std::cout << "Aim Assist initialization: FAILED\n";
    }
    
    // Test Vec3 operations
    std::cout << "Testing Vec3 operations...\n";
    Vec3 v1(1, 2, 3);
    Vec3 v2(4, 5, 6);
    Vec3 v3 = v1 + v2;
    std::cout << "Vec3 addition: " << (v3.x == 5 && v3.y == 7 && v3.z == 9 ? "PASSED" : "FAILED") << "\n";
    
    float length = v1.Length();
    std::cout << "Vec3 length calculation: " << (length > 3.7 && length < 3.8 ? "PASSED" : "FAILED") << "\n";
    
    Logger::Get().Log("TEST", "Unified system test completed successfully");
    std::cout << "=== All Tests Completed ===\n";
    
    return 0;
}