add_library(Utils 
    Logger.cpp 
    GameDetection.cpp
    UniversalConfig.cpp
    UniversalMemoryScanner.cpp
    UnifiedAimAssist.cpp
    StringConvert.h
)
target_include_directories(Utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Enable position-independent code for use in shared libraries
set_target_properties(Utils PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Add system libraries for Windows API calls in cross-platform way
if(WIN32)
    target_link_libraries(Utils PRIVATE psapi)
endif()

if(MSVC)
    target_compile_options(Utils PRIVATE /W4 /WX /EHsc)
else()
    target_compile_options(Utils PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()
