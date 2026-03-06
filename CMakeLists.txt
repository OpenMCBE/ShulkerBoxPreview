cmake_minimum_required(VERSION 3.18)
project(ShulkerPreview LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -fexceptions -w")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -w")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections,--strip-all -s")

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG main
)
FetchContent_GetProperties(preloader_android)
if(NOT preloader_android_POPULATED)
    FetchContent_Populate(preloader_android)

    file(READ "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" LOGGER_CONTENT)
    string(REPLACE "#include <format>"         "#include <fmt/format.h>"      LOGGER_CONTENT "${LOGGER_CONTENT}")
    string(REPLACE "std::vformat"              "fmt::vformat"                  LOGGER_CONTENT "${LOGGER_CONTENT}")
    string(REPLACE "std::make_format_args"     "fmt::make_format_args"         LOGGER_CONTENT "${LOGGER_CONTENT}")
    file(WRITE "${preloader_android_SOURCE_DIR}/src/pl/Logger.h" "${LOGGER_CONTENT}")

    add_subdirectory(${preloader_android_SOURCE_DIR} ${preloader_android_BINARY_DIR})
endif()

target_link_libraries(preloader PUBLIC fmt::fmt)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    simpleini
    GIT_REPOSITORY https://github.com/brofield/simpleini.git
    GIT_TAG master
)
FetchContent_MakeAvailable(simpleini)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ImGui
    ${CMAKE_SOURCE_DIR}/src/ImGui/backends
    ${simpleini_SOURCE_DIR}
)

file(GLOB_RECURSE GAME_SOURCES
    src/*.cpp
)
# Exclude ImGui demo (not needed in release)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*imgui_demo\\.cpp$")

add_library(ShulkerPreview SHARED ${GAME_SOURCES})
target_include_directories(ShulkerPreview PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ImGui
    ${CMAKE_SOURCE_DIR}/src/ImGui/backends
    ${simpleini_SOURCE_DIR}
)
target_link_libraries(ShulkerPreview
    preloader
    glm::glm
    log
    android
    EGL
    GLESv3
    GLESv2
)
