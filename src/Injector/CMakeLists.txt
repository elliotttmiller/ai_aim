# src/Injector/CMakeLists.txt
add_executable(Injector main.cpp ../Overlay/IPC/SharedMemory.cpp ../Overlay/IPC/NamedPipe.cpp)

# Set output directories for Debug/Release
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

# Link against necessary Windows libraries
# Use PRIVATE for implementation dependencies
target_link_libraries(Injector PRIVATE user32 kernel32)

# Add post-build step to copy Injector.exe to bin/Debug or bin/Release
add_custom_command(TARGET Injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Injector>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/bin/Debug/Injector.exe,${CMAKE_SOURCE_DIR}/bin/Release/Injector.exe>
)
