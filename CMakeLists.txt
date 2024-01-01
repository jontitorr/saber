cmake_minimum_required(VERSION 3.16)

project(
    "saber"
    VERSION 0.1.0
    LANGUAGES CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

if(UNIX)
    add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")
endif()

option(saber_INSTALL "Generate the install target" ON)

include(cmake/CPM.cmake)

set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/bin") # .exe and
                                                                   # .dll
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/lib") # .so and
                                                                   # .dylib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<0:>${CMAKE_BINARY_DIR}/lib") # .lib and .a

cpmaddpackage(NAME "ekizu" GITHUB_REPOSITORY "Xminent/ekizu" GIT_TAG "dev")

cpmfindpackage(
    NAME
    "spdlog"
    GITHUB_REPOSITORY
    "gabime/spdlog"
    GIT_TAG
    "v1.12.0"
    OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "include/*.hpp" "src/*.cpp")
list(FILTER sources EXCLUDE REGEX ".*main\\.cpp$")

# Actual Logic

add_library(saber_lib ${sources})

include(GenerateExportHeader)

generate_export_header(
    saber_lib EXPORT_FILE_NAME export/saber/export.h EXPORT_MACRO_NAME
    SABER_EXPORT
)

target_compile_definitions(
    saber_lib PUBLIC $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:SABER_STATIC_DEFINE>
)

target_include_directories(
    saber_lib
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>
           $<INSTALL_INTERFACE:include>
)

target_link_libraries(saber_lib PUBLIC ekizu spdlog::spdlog Boost::url)

# EXE

add_executable(saber src/main.cpp)

if(WIN32)
    target_compile_definitions(saber PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(saber PRIVATE saber_lib)

# Command Setup

file(GLOB_RECURSE commands CONFIGURE_DEPENDS commands/*.cpp)

foreach(fullcmdname ${commands})
    get_filename_component(cmdname ${fullcmdname} NAME_WE)
    message(STATUS "Found command:'cmd_${cmdname}'")
    add_library(cmd_${cmdname} SHARED ${fullcmdname})
    target_link_libraries(cmd_${cmdname} PRIVATE saber_lib)
endforeach(fullcmdname)

if(saber_INSTALL)
    install(TARGETS saber DESTINATION bin)
endif()
