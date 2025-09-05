cmake_minimum_required(VERSION 3.16)
project(ai_aim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Debug)

# Define missing JoinPaths function for raylib
function(join_paths joined_path first_path_segment)
    set(temp_path "${first_path_segment}")
    foreach(current_segment IN LISTS ARGN)
        if(NOT ("${current_segment}" STREQUAL ""))
            if(IS_ABSOLUTE "${current_segment}")
                set(temp_path "${current_segment}")
            else()
                set(temp_path "${temp_path}/${current_segment}")
            endif()
        endif()
    endforeach()
    set(${joined_path} "${temp_path}" PARENT_SCOPE)
endfunction()

# For the user's AimTrainer game development, enable raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE) 
set(SUPPORT_FILEFORMAT_FLAC OFF CACHE BOOL "" FORCE)
set(PLATFORM_DESKTOP ON CACHE BOOL "" FORCE)
# Disable X11 and Wayland for CI environments
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

# Build raylib for AimTrainer
add_subdirectory(libs/raylib)

# Build minhook (Windows-specific - disable for cross-platform development)
# if(WIN32)
#     add_subdirectory(libs/minhook)
# endif()

# Build dear-imgui
# add_subdirectory(libs/dear-imgui)

# Build Utils and IPC modules if they have their own CMakeLists.txt
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Utils/CMakeLists.txt")
    add_subdirectory(src/Utils)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/src/IPC/CMakeLists.txt")
    add_subdirectory(src/IPC)
endif()

# Build Overlay (Windows only, but enable for cross-platform testing)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Overlay/CMakeLists.txt")
    add_subdirectory(src/Overlay)
endif()

# Build Injector
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Injector/CMakeLists.txt")
    add_subdirectory(src/Injector)
endif()

# Build Launcher
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Launcher/CMakeLists.txt")
    add_subdirectory(src/Launcher)
endif()

# Build InjectedDLL (Windows only, but enable for cross-platform testing)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/InjectedDLL/CMakeLists.txt")
    add_subdirectory(src/InjectedDLL)
endif()

# Build AimTrainer - the user's training game
if(EXISTS "${CMAKE_SOURCE_DIR}/src/AimTrainer/CMakeLists.txt")
    add_subdirectory(src/AimTrainer)
endif()
