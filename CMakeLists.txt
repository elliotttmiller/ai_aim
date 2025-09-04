cmake_minimum_required(VERSION 3.16)
project(ai_aim_universal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Build raylib (includes GLFW)
add_subdirectory(libs/raylib/src)

# Build minhook
add_subdirectory(libs/minhook)

# Build dear-imgui
add_subdirectory(libs/dear-imgui)

# Build Utils module (required by Universal)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Utils/CMakeLists.txt")
    add_subdirectory(src/Utils)
endif()

# Build Universal module (required by all other modules)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Universal/CMakeLists.txt")
    add_subdirectory(src/Universal)
endif()

# Build IPC module
if(EXISTS "${CMAKE_SOURCE_DIR}/src/IPC/CMakeLists.txt")
    add_subdirectory(src/IPC)
endif()

# Build Overlay (depends on Universal)
add_subdirectory(src/Overlay)

# Build Injector (depends on Universal)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Injector/CMakeLists.txt")
    add_subdirectory(src/Injector)
endif()

# Build Launcher (depends on Universal)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Launcher/CMakeLists.txt")
    add_subdirectory(src/Launcher)
endif()

# Build AimTrainer (test application)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/AimTrainer/CMakeLists.txt")
    add_subdirectory(src/AimTrainer)
endif()

# Build InjectedDLL (legacy test)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/InjectedDLL/CMakeLists.txt")
    add_subdirectory(src/InjectedDLL)
endif()
