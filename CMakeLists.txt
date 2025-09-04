cmake_minimum_required(VERSION 3.15)
project(ExternalAimAssist)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_library(d3d12helpers INTERFACE)
target_include_directories(d3d12helpers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/d3d12helpers/include)

add_subdirectory(libs/minhook)
add_subdirectory(libs/raylib)
add_subdirectory(libs/dear-imgui)
add_subdirectory(src/AimTrainer)
add_subdirectory(src/Launcher)
add_subdirectory(src/Injector)
add_subdirectory(src/Overlay)
add_subdirectory(src/Utils)
add_subdirectory(src/InjectedDLL)
add_subdirectory(src/IPC)
