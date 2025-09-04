# src/Universal/CMakeLists.txt
add_library(Universal STATIC
    GameDetection.cpp
    MemoryScanner.h
    GraphicsDetection.h
    InjectionManager.h
    AimSystem.h
)

# Set output directory for all configs
set_target_properties(Universal PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Include directories
target_include_directories(Universal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/libs/dear-imgui
)

# Link dependencies
target_link_libraries(Universal PRIVATE
    Utils
    psapi
    advapi32
    kernel32
    user32
)

# Compiler-specific settings
if(MSVC)
    target_compile_options(Universal PRIVATE /W4 /EHsc)
else()
    target_compile_options(Universal PRIVATE -Wall -Wextra -pedantic)
endif()