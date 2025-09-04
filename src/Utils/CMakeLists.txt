add_library(Utils Logger.cpp StringConvert.h)
target_include_directories(Utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(Utils PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
)
if(MSVC)
    target_compile_options(Utils PRIVATE /W4 /WX /EHsc)
else()
    target_compile_options(Utils PRIVATE -Wall -Wextra -pedantic $<$<CONFIG:Debug>:-Werror>)
endif()
