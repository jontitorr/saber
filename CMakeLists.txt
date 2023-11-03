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

option(${PROJECT_NAME}_INSTALL "Generate the install target" ON)

include(cmake/CPM.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cpmfindpackage(
    NAME
    "ekizu"
    GITHUB_REPOSITORY
    "Xminent/ekizu"
    GIT_TAG
    "dev"
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
)

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

add_executable(${PROJECT_NAME} ${sources})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(
    ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE ekizu spdlog::spdlog)

file(GLOB_RECURSE commands CONFIGURE_DEPENDS commands/*.cpp)

foreach(fullcmdname ${commands})
    get_filename_component(cmdname ${fullcmdname} NAME_WE)
    message(STATUS "Found command:'cmd_${cmdname}'")
    add_library(cmd_${cmdname} SHARED ${fullcmdname})
    target_include_directories(
        cmd_${cmdname} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cmd_${cmdname} PRIVATE ekizu)
endforeach(fullcmdname)

if(${PROJECT_NAME}_INSTALL)
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=thread)
target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=thread)
