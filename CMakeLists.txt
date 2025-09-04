cmake_minimum_required(VERSION 3.16)
project(ai_aim)

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

# Build Utils and IPC modules if they have their own CMakeLists.txt
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Utils/CMakeLists.txt")
    add_subdirectory(src/Utils)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/src/IPC/CMakeLists.txt")
    add_subdirectory(src/IPC)
endif()

# Build Overlay
add_subdirectory(src/Overlay)

# Build Injector
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Injector/CMakeLists.txt")
    add_subdirectory(src/Injector)
endif()

# Build Launcher
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Launcher/CMakeLists.txt")
    add_subdirectory(src/Launcher)
endif()

# Build InjectedDLL
if(EXISTS "${CMAKE_SOURCE_DIR}/src/InjectedDLL/CMakeLists.txt")
    add_subdirectory(src/InjectedDLL)
endif()
