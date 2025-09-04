// src/Overlay/Core/Main.h
#pragma once
#include <atomic>

namespace Main {
    // Global flag to control the main loop. Atomic for thread safety.
    inline std::atomic<bool> g_bRunning = true;

    // The main function that will run in its own thread.
    void MainLoop();
}
