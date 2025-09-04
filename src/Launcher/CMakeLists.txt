add_executable(Launcher Launcher.cpp)
target_include_directories(Launcher PRIVATE ../Utils)
target_link_libraries(Launcher PRIVATE Utils IPC)
set_target_properties(Launcher PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)

# Enable strict warnings and treat warnings as errors in Debug
if(MSVC)
    target_compile_options(Launcher PRIVATE /W4 $<$<CONFIG:Debug>:/WX>)
else()
    target_compile_options(Launcher PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()

add_custom_command(TARGET Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Launcher>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/bin/Debug/Launcher.exe,${CMAKE_SOURCE_DIR}/bin/Release/Launcher.exe>
)
