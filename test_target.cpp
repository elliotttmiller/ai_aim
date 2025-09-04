#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <unistd.h>

// Simple test target application that the AI_AIM system can detect and test against
int main() {
    std::cout << "AI_AIM Test Target Application" << std::endl;
    std::cout << "This application simulates a game process for testing the aim assist system." << std::endl;
    std::cout << "Process ID: " << getpid() << std::endl;
    std::cout << "Running for 60 seconds... (Press Ctrl+C to exit)" << std::endl;
    
    // Simulate a simple game loop
    int frame = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    // Create persistent game data structure
    struct GameData {
        float playerX;
        float playerY;
        float playerZ;
        float health;
        bool alive;
    };
    
    GameData gameData = {100.0f, 200.0f, 0.0f, 100.0f, true};
    
    while (frame < 3600) { // Run for 60 seconds at 60fps
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - startTime).count();
        
        if (frame % 300 == 0) { // Print status every 5 seconds
            std::cout << "Frame: " << frame << ", Time: " << elapsed << "s" 
                      << ", Player: (" << gameData.playerX << ", " << gameData.playerY << ")" << std::endl;
        }
        
        // Update game data
        gameData.playerX = 100.0f + frame * 0.1f;
        gameData.playerY = 200.0f + frame * 0.05f;
        
        frame++;
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }
    
    std::cout << "Test target application exiting normally." << std::endl;
    return 0;
}