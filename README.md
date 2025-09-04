# ai_aim: Universal Autonomous Game Overlay & Integration System

**Version:** 1.1.0 (December 2024)

## Overview

**ai_aim** is a robust, modular, and extensible system for autonomous overlay, aim assist, and real-time data integration with any Windows game. The architecture is designed for universal use—supporting injection, overlay, and configuration for any game, not just the included Aim Trainer. The workflow is fully automated, requiring no manual code changes to the target game, and is adaptable for a wide range of use cases (aim assist, overlays, analytics, etc.).

---

## System Architecture

```
ai_aim/
├── bin/                # Compiled binaries and debug logs
│   ├── Debug/          # Debug build outputs
│   ├── debug.log       # Unified debug log for all modules
│   └── ...             # All .exe, .dll, .pdb, .lib files
├── config/             # Configuration files
│   └── game_memory.cfg # Memory layout/config for overlay/IPC
├── libs/               # External dependencies (submodules)
│   ├── dear-imgui/     # ImGui GUI library
│  │   ├── minhook/        # MinHook for function hooking
│   └── raylib/         # Raylib graphics library
├── src/                # All source code
│   ├── AimTrainer/     # Standalone FPS aim trainer (test app)
│   │   ├── main.cpp    # Main game logic and entry point
│   │   └── CMakeLists.txt
│   ├── InjectedDLL/    # Legacy DLL for testing injection
│   │   ├── dllmain.cpp # Simple DLL entry point
│   │   └── CMakeLists.txt
│   ├── Injector/       # DLL injector for overlay
│   │   ├── main.cpp    # Injector logic and IPC setup
│   │   └── CMakeLists.txt
│   ├── IPC/            # Shared IPC code (shared memory only)
│   │   ├── SharedMemory.cpp/.h    # Shared memory IPC implementation
│   │   ├── SharedStructs.h        # IPC data structures
│   │   └── CMakeLists.txt
│   ├── Launcher/       # Orchestrates workflow, launches and injects
│   │   ├── Launcher.cpp# Main launcher logic
│   │   └── CMakeLists.txt
│   ├── Overlay/        # Main overlay DLL (aim assist, UI, hooks)
│   │   ├── Core/       # DLL entry, main loop
│   │   │   ├── DllMain.cpp    # DLL entry point, error handling, logging
│   │   │   ├── Main.cpp/.h    # Overlay main loop, ImGui, hooks
│   │   ├── AimAssist/  # Aim assist logic
│   │   │   ├── AimAssist.cpp/.h # Target detection, aim logic
│   │   ├── Input/      # Input management
│   │   │   ├── InputManager.cpp/.h # Keyboard/mouse input
│   │   ├── IPC/        # Overlay-side IPC
│   │   │   ├── SharedMemory.cpp/.h # Shared memory communication
│   │   ├── Memory/     # Game memory scanning
│   │   │   ├── GameData.h     # Game data structures and scanning
│   │   ├── Renderer/   # Overlay rendering
│   │   │   ├── Renderer.cpp/.h    # Custom rendering logic
│   │   ├── UI/         # Overlay UI (ImGui menus)
│   │   │   ├── Menu.cpp/.h        # Menu and UI logic
│   │   ├── Utils/      # Utility code
│   │   │   ├── Singleton.h        # Singleton pattern helper
│   │   └── CMakeLists.txt
│   └── Utils/          # Shared utility code
│       ├── Logger.cpp/.h      # Unified logging system
│       ├── Singleton.h        # Singleton pattern helper
│       ├── StringConvert.h    # String conversion utilities
│       └── CMakeLists.txt
├── x64/                # Visual Studio build artifacts
├── .gitignore          # Excludes build, IDE, and log files
├── .gitmodules         # Git submodule configuration
├── CMakeLists.txt      # Top-level CMake build configuration
└── README.md           # Project documentation (this file)
```

---

## Universal Autonomous Workflow & Integration

### 1. Launcher (`Launcher.exe`)
- Orchestrates the entire workflow for any target game.
- Launches the specified game executable, waits for its process, and injects the overlay DLL using the Injector.
- Handles process lifecycle, error handling, and configuration automatically.
- All actions are logged to `bin/debug.log`.

### 2. Injector (`Injector.exe`)
- Finds the target game process and injects the overlay DLL (`Overlay.dll`) using configurable methods (SetWindowsHookEx, CreateRemoteThread, etc.).
- Sets up shared memory IPC channels for high-performance communication between overlay and game.
- Logs all injection and IPC setup events to `bin/debug.log`.

### 3. Overlay DLL (`Overlay.dll`)
- Injected into any game process, starts in `DllMain.cpp`.
- Main loop (`Main.cpp`) sets up ImGui, manages overlay rendering, and coordinates all modules.
- Modular architecture with separate components for aim assist, input management, memory scanning, and UI.
- Uses shared memory IPC for real-time data exchange with minimal latency.
- All overlay actions, errors, and events are logged to `bin/debug.log`.

### 4. Unified Debug Logging
- All modules write their debug output to `bin/debug.log` for easy troubleshooting and diagnostics.
- Centralized logging system via Utils/Logger for consistent formatting and thread safety.

### 5. Configuration & Extensibility
- The system uses shared memory IPC with standardized data structures defined in `SharedStructs.h`.
- Modular CMake build system allows easy addition of new components.
- New overlays, assist modules, or analytics can be added without changing the target game code.
- Configuration files in `config/` directory for game-specific memory layouts and settings.

---

## Example: Aim Trainer Tester App

To demonstrate and test the universal overlay/injection system, we created a standalone FPS Aim Trainer app:

- **AimTrainer.exe**: A Raylib-powered FPS aim trainer that spawns targets, tracks user input, and exposes memory anchors for overlay scanning.
- The overlay DLL and injector work with AimTrainer exactly as they would with any other game—no code changes required.
- All key events (initialization, target spawn, anchor creation) are logged to `bin/debug.log`.
- This tester app validates the universal workflow and integration for any game.

---

## DLL Injection Methods

**SetWindowsHookEx Injection**
- Utilizes the Windows API to install a hook procedure into the target game process, allowing the DLL to be loaded automatically when specific system events occur (e.g., keyboard, mouse, or window messages).
- This method is stealthy and compatible with many games, as it leverages legitimate OS mechanisms.
- Once injected, the DLL can access the game’s memory space, read relevant data (such as enemy positions), and write results to shared memory or named pipes for use by external overlays.
- Ideal for overlays, input interception, and lightweight integrations where minimal disruption to the game process is required.

**Manual DLL Injection**
- Directly injects the DLL into the target process using `CreateRemoteThread` and `LoadLibrary`.
- The injector opens the game process, allocates memory for the DLL path, writes the path, and creates a remote thread to load the DLL.
- This method is fast, reliable, and works with most Windows games, but typically requires administrator privileges due to process access rights.
- Preferred for development, debugging, and scenarios where full control over the injection process is needed.

**Process Hollowing / Code Cave**
- An advanced technique where the injector replaces or augments the code of a running process with custom code or DLL entry points.
- The original process is started in a suspended state, its memory is modified to inject the desired code, and then resumed.
- Enables deep integration, custom loaders, and anti-detection strategies, but is more complex and riskier than standard injection methods.
- Used in security research, advanced overlays, and scenarios requiring complete control over process execution.

---

## Architecture Overview

**Your Game Process:**
```
├── Original Game Code (untouched)
├── Injected DLL
│   ├── Scans and reads game memory for real-time data (e.g., enemy positions, player stats)
│   ├── Communicates with external modules via shared memory or named pipes
│   └── Maintains a minimal footprint, ensuring no modification to game logic or assets
```

**External Overlay Process:**
```
├── Overlay/Assist/Analytics System (completely separate from the game)
├── Reads live target and game data from shared communication channels
├── Renders overlays, applies aim assist, or performs analytics based on real game state
└── Can be updated, extended, or distributed independently of the game
```

---

## Benefits of This Approach

- **Zero source code changes to your game**: No need to modify, patch, or recompile the game itself. All integration is external and non-invasive.
- **High precision**: Direct access to in-memory game data (such as enemy coordinates, player stats, etc.) enables accurate overlays and assist features.
- **Real-time data**: Memory scanning and IPC provide instant access to game state, eliminating the lag and inaccuracies of screen capture or input polling.
- **Completely external**: The overlay, aim assist, and analytics modules are developed and maintained separately, allowing for rapid iteration and easy updates.
- **Investor-friendly**: No exposure or redistribution of proprietary game code. The system is safe for demonstration, review, and deployment in professional environments.
- **Universal compatibility**: The architecture supports any Windows game, making it suitable for a wide range of use cases beyond aim training (e.g., analytics, accessibility, custom overlays).

---

## Getting Started

1. **Clone the repository** (with submodules):
   ```sh
   git clone --recurse-submodules https://github.com/elliotttmiller/ai_aim.git
   ```
2. **Configure and build** using CMake or Visual Studio 2022:
   - All build outputs are placed in `bin/Debug/`
   - Use the root `CMakeLists.txt` for full project build
   - Individual modules can be built separately if needed
3. **Run `Launcher.exe`** from `bin/Debug/` to start the full workflow
4. **Check `bin/debug.log`** for unified debug output and diagnostics
5. **Note**: AimTrainer test app currently requires build configuration fixes

---

## Contributing & Extending

- The project is modular—new overlays, aim assist algorithms, or IPC methods can be added easily.
- All code is organized for clarity and maintainability, with robust error handling and logging.
- See each module's source files for further documentation and extension points.

---

## License

This project uses open-source libraries (Raylib, ImGui, MinHook) under their respective licenses. See `libs/` for details.

---

## Current Status & Development Notes

### Current Implementation Status
- **Build System**: CMake-based modular build system with all outputs in `bin/Debug/`
- **IPC**: Shared memory implementation only (legacy named pipe code removed)
- **Injection**: Manual DLL injection via `CreateRemoteThread` and `LoadLibrary`
- **Overlay**: ImGui-based overlay with modular architecture
- **Logging**: Unified logging system via `Utils/Logger` to `bin/debug.log`
- **Test App**: AimTrainer not currently building (needs CMakeLists.txt configuration)

### Known Issues & Optimizations Needed
- **AimTrainer Build**: Test application needs proper CMake configuration to build
- **Overlay/Aim Assist Logic**: Core overlay and aim assist functionality not currently working
- **Path Configuration**: Some hardcoded paths need to be made dynamic via config files
- **Error Handling**: Enhanced error handling and recovery mechanisms needed
- **Performance**: Memory scanning and overlay rendering optimization required
- **Anti-Detection**: Injection methods need hardening against anti-cheat systems

### Next Steps for Production Ready System
1. **Fix Overlay/Aim Assist Logic**: Implement working overlay rendering and aim assist algorithms
2. **Complete AimTrainer Build**: Fix CMake configuration for test application
3. **Dynamic Configuration**: Move all hardcoded paths to `config/game_memory.cfg`
4. **Robust Error Handling**: Implement comprehensive error recovery and logging
5. **Performance Optimization**: Profile and optimize memory scanning and rendering
6. **Security Hardening**: Implement anti-detection measures for injection
7. **Documentation**: Add detailed API documentation and usage examples

---

## Contact

For questions, issues, or contributions, open an issue or pull request on GitHub.
