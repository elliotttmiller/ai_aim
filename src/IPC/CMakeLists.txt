cmake_minimum_required(VERSION 3.15)
project(IPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(IPC STATIC
    NamedPipe.cpp
    SharedMemory.cpp
)

# Include headers for IPC
 target_include_directories(IPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
