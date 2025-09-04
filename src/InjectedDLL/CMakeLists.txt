add_library(InjectedDLL SHARED dllmain.cpp)
target_include_directories(InjectedDLL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../IPC ../Utils)
target_link_libraries(InjectedDLL PRIVATE Utils IPC)
set_target_properties(InjectedDLL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)
if(MSVC)
    target_compile_options(InjectedDLL PRIVATE /W4 $<$<CONFIG:Debug>:/WX>)
else()
    target_compile_options(InjectedDLL PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()
