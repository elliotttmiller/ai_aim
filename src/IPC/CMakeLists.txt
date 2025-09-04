cmake_minimum_required(VERSION 3.16)
project(IPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(IPC STATIC
    SharedMemory.cpp
)

# Include headers for IPC
 target_include_directories(IPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(IPC PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)
if(MSVC)
    target_compile_options(IPC PRIVATE /W4 /WX /EHsc)
else()
    target_compile_options(IPC PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()
