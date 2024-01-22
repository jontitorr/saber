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

set(TRY_BOOST_VERSION "1.81.0")
set(BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED
    "coroutine;filesystem;thread;url"
)
set(BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED
    "asio;beast;process;scope_exit;outcome"
)

set(IS_BOOST_LOCAL OFF)
if(${CPM_LOCAL_PACKAGES_ONLY})
    message(STATUS "Trying to find Boost...")
    find_package(
        Boost ${TRY_BOOST_VERSION} REQUIRED
        COMPONENTS ${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}
    )
    set(IS_BOOST_LOCAL ON)
elseif(${CPM_USE_LOCAL_PACKAGES} OR NOT ${CPM_DOWNLOAD_ALL})
    message(STATUS "Trying to use local Boost...")
    find_package(
        Boost ${TRY_BOOST_VERSION}
        COMPONENTS ${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}
    )
    if(${BOOST_FOUND})
        set(IS_BOOST_LOCAL ON)
        message(DEBUG "boost include dir: ${Boost_INCLUDE_DIRS}")
    endif()
endif()

if(NOT (${BOOST_FOUND}) OR (NOT DEFINED BOOST_FOUND))
    message(STATUS "Trying to download Boost...")
    set(BOOST_INCLUDE_LIBRARIES
        "${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED};${BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}"
    )
    set(BOOST_SKIP_INSTALL_RULES OFF)
    cpmaddpackage(
        NAME
        Boost
        URL
        "https://github.com/jontitorr/boost-cmake/releases/download/1.84.0-release/boost-1.84.0.tar.xz"
    )
    set(IS_BOOST_LOCAL OFF)
endif()

cpmaddpackage(NAME "ekizu" GITHUB_REPOSITORY "jontitorr/ekizu" GIT_TAG "dev")

cpmfindpackage(
    NAME
    "Ogg"
    GITHUB_REPOSITORY
    "xiph/ogg"
    GIT_TAG
    "v1.3.5"
    OPTIONS
    "BUILD_TESTING OFF"
)

cpmfindpackage(
    NAME
    "opusfile"
    GITHUB_REPOSITORY
    "xiph/opusfile"
    GIT_TAG
    "master"
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
    "OP_DISABLE_HTTP ON"
    "OP_DISABLE_EXAMPLES ON"
    "OP_DISABLE_DOCS ON"
)

set_property(TARGET opusfile PROPERTY POSITION_INDEPENDENT_CODE ON)

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

target_link_libraries(
    saber_lib PUBLIC ekizu OpusFile::opusfile spdlog::spdlog Boost::url
)

if(NOT ${IS_BOOST_LOCAL})
    target_link_libraries(saber_lib PUBLIC Boost::process Boost::scope_exit)
endif()

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
