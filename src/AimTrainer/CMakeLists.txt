# --- START OF FILE src/AimTrainer/CMakeLists.txt ---
add_executable(AimTrainer main.cpp)
target_include_directories(AimTrainer PRIVATE ../../libs/raylib/src)
target_link_libraries(AimTrainer PRIVATE raylib)
set_target_properties(AimTrainer PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

add_custom_command(TARGET AimTrainer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AimTrainer>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/bin/Debug/AimTrainer.exe,${CMAKE_SOURCE_DIR}/bin/Release/AimTrainer.exe>
)
# --- END OF FILE src/AimTrainer/CMakeLists.txt ---
