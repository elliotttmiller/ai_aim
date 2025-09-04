# src/Overlay/CMakeLists.txt
add_library(Overlay STATIC
    AimAssist/UniversalAimAssist.cpp
)

# Set output directory for all configs
set_target_properties(Overlay PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Enable strict warnings and treat warnings as errors in Debug
if(MSVC)
    target_compile_options(Overlay PRIVATE /W4 /WX /EHsc)
else()
    target_compile_options(Overlay PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()

# Include directories for our project and libs
target_include_directories(Overlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs/dear-imgui
    ${CMAKE_SOURCE_DIR}/libs/dear-imgui/backends
    ${CMAKE_SOURCE_DIR}/libs/minhook/include
    ${CMAKE_SOURCE_DIR}/libs/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/IPC
    ${CMAKE_SOURCE_DIR}/src/Utils
)

# Link against necessary libraries
target_link_libraries(Overlay PRIVATE
    # Platform-specific libraries (commented out for core testing)
    # $<$<PLATFORM_ID:Windows>:dxgi>
    # $<$<PLATFORM_ID:Windows>:opengl32>
    # $<$<PLATFORM_ID:Linux>:GL>
    # $<$<PLATFORM_ID:Linux>:X11>
    Utils
)

# Post-build: Copy Overlay.dll to bin/Debug for launcher/injector
add_custom_command(TARGET Overlay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Overlay>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/bin/Debug/Overlay.dll,${CMAKE_SOURCE_DIR}/bin/Release/Overlay.dll>
)
