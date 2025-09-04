# ai_aim: Universal Autonomous Game Overlay & Integration System

**Version:** 1.0.0 (September 2025)

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
│   ├── minhook/        # MinHook for function hooking
│   └── raylib/         # Raylib graphics library
├── out/                # CMake and build system output
├── src/                # All source code
│   ├── AimTrainer/     # Standalone FPS aim trainer
│   │   ├── main.cpp    # Main game logic and entry point
│   │   └── ...         # Project files, CMake, filters
│   ├── Injector/       # DLL injector for overlay
│   │   ├── main.cpp    # Injector logic and IPC setup
│   │   └── ...         # Project files, CMake, filters
│   ├── IPC/            # Shared IPC code
│   │   ├── NamedPipe.cpp/.h   # Named pipe IPC implementation
│   │   ├── SharedMemory.cpp/.h# Shared memory IPC implementation
│   ├── Launcher/       # Orchestrates workflow, launches and injects
│   │   ├── Launcher.cpp# Main launcher logic
│   │   └── ...         # Project files, CMake, filters
│   ├── Overlay/        # Overlay DLL (aim assist, UI, hooks)
│   │   ├── Core/       # DLL entry, main loop
│   │   │   ├── DllMain.cpp    # DLL entry point, error handling, logging
│   │   │   ├── Main.cpp/.h    # Overlay main loop, ImGui, hooks
│   │   ├── AimAssist/  # Aim assist logic
│   │   │   ├── AimAssist.cpp/.h # Target detection, aim logic
│   │   ├── Hooks/      # Graphics API hooks
│   │   │   ├── D3D11Hook.cpp/.h # Direct3D 11 hook implementation
│   │   ├── Input/      # Input management
│   │   │   ├── InputManager.cpp/.h # Keyboard/mouse input
│   │   ├── IPC/        # Overlay-side IPC
│   │   │   ├── NamedPipe.cpp/.h
│   │   │   ├── SharedMemory.cpp/.h
│   │   ├── Memory/     # Game memory scanning
│   │   │   ├── GameData.cpp/.h    # Scans and exposes game data
│   │   ├── Renderer/   # Overlay rendering
│   │   │   ├── Renderer.cpp/.h    # Custom rendering logic
│   │   ├── UI/         # Overlay UI (ImGui menus)
│   │   │   ├── Menu.cpp/.h        # Menu and UI logic
│   │   ├── Utils/      # Utility code
│   │   │   ├── Singleton.h        # Singleton pattern helper
│   │   └── ...         # Project files, CMake, filters
├── .gitignore          # Excludes build, IDE, and log files
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
- Sets up IPC channels (named pipes/shared memory) for communication between overlay and game.
- Logs all injection and IPC setup events to `bin/debug.log`.

### 3. Overlay DLL (`Overlay.dll`)
- Injected into any game process, starts in `DllMain.cpp`.
- Main loop (`Main.cpp`) sets up ImGui, hooks graphics API (D3D11), and manages overlay rendering.
- Custom logic modules (aim assist, analytics, UI, etc.) scan game memory, detect targets/events, and apply algorithms.
- IPC modules exchange data with the game for real-time overlay and integration.
- All overlay actions, errors, and events are logged to `bin/debug.log`.

### 4. Unified Debug Logging
- All modules write their debug output to `bin/debug.log` for easy troubleshooting and diagnostics.

### 5. Configuration & Extensibility
- The system is fully configurable for any game: memory layouts, injection methods, overlay features, and IPC can be adapted via config files and modular code.
- New overlays, assist modules, or analytics can be added without changing the target game code.

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
2. **Configure and build** using CMake or Visual Studio 2022.
3. **Run `Launcher.exe`** from `bin/Debug` to start the full workflow.
4. **Check `bin/debug.log`** for unified debug output and diagnostics.

---

## Contributing & Extending

- The project is modular—new overlays, aim assist algorithms, or IPC methods can be added easily.
- All code is organized for clarity and maintainability, with robust error handling and logging.
- See each module's source files for further documentation and extension points.

---

## License

This project uses open-source libraries (Raylib, ImGui, MinHook) under their respective licenses. See `libs/` for details.

---

## Contact

For questions, issues, or contributions, open an issue or pull request on GitHub.
