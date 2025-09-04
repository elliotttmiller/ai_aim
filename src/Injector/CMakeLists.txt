cmake_minimum_required(VERSION 3.15)
project(Injector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Injector main.cpp)

# Link with Utils for Logger and string conversion
find_package(Threads REQUIRED)
target_link_libraries(Injector PRIVATE Utils IPC Threads::Threads)
target_include_directories(Injector PRIVATE ${CMAKE_SOURCE_DIR}/src/Overlay/IPC)

# Output directories
set_target_properties(Injector PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Enable strict warnings and treat warnings as errors in Debug
if(MSVC)
    target_compile_options(Injector PRIVATE /W4 $<$<CONFIG:Debug>:/WX>)
else()
    target_compile_options(Injector PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()
