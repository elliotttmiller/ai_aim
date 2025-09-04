# src/AimTrainer/CMakeLists.txt
add_executable(AimTrainer main.cpp)

# Include directories
target_include_directories(AimTrainer PRIVATE 
    ${CMAKE_SOURCE_DIR}/libs/raylib/src
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/src/IPC
)

# Link with raylib and dependencies
target_link_libraries(AimTrainer PRIVATE 
    raylib 
    Utils
    IPC
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(AimTrainer PRIVATE winmm gdi32 opengl32)
endif()

# Output directories
set_target_properties(AimTrainer PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Compiler settings
if(MSVC)
    target_compile_options(AimTrainer PRIVATE /W4 /EHsc)
else()
    target_compile_options(AimTrainer PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy executable to bin directory
add_custom_command(TARGET AimTrainer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AimTrainer>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/bin/Debug/AimTrainer.exe,${CMAKE_SOURCE_DIR}/bin/Release/AimTrainer.exe>
)
