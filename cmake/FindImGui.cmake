# FindImGui.cmake — Locate Dear ImGui
#
# Targets:
#   imgui::imgui       — core library
#   imgui::imgui_impl  — all back-ends found
#
# Hints: set ImGui_ROOT to the directory containing the imgui/ source.

if(TARGET imgui::imgui)
    return()
endif()

# When cross-compiling with CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY, find_path
# only searches the sysroot.  Temporarily allow BOTH so we can find host-side
# headers in the project tree.
set(_saved_frp_mode ${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

find_path(ImGui_INCLUDE_DIR
    NAMES imgui.h
    PATHS
        "${ImGui_ROOT}"
        "$ENV{ImGui_ROOT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui"
    PATH_SUFFIXES imgui
    NO_DEFAULT_PATH
)

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${_saved_frp_mode})

if(NOT ImGui_INCLUDE_DIR)
    message(FATAL_ERROR
        "Dear ImGui not found. Set ImGui_ROOT or place the source in "
        "third_party/imgui/")
endif()

file(GLOB IMGUI_SOURCES
    "${ImGui_INCLUDE_DIR}/*.cpp"
    "${ImGui_INCLUDE_DIR}/backends/imgui_impl_win32.cpp"
    "${ImGui_INCLUDE_DIR}/backends/imgui_impl_dx11.cpp"
    "${ImGui_INCLUDE_DIR}/misc/cpp/imgui_stdlib.cpp"
)

file(GLOB IMGUI_HEADERS
    "${ImGui_INCLUDE_DIR}/*.h"
    "${ImGui_INCLUDE_DIR}/backends/imgui_impl_win32.h"
    "${ImGui_INCLUDE_DIR}/backends/imgui_impl_dx11.h"
)

add_library(imgui_core STATIC
    ${IMGUI_SOURCES}
    ${IMGUI_HEADERS}
)

target_include_directories(imgui_core PUBLIC "${ImGui_INCLUDE_DIR}")
target_compile_definitions(imgui_core PRIVATE IMGUI_IMPL_API=)
if(MSVC)
    target_compile_options(imgui_core PRIVATE /utf-8)
endif()

add_library(imgui::imgui ALIAS imgui_core)

message(STATUS "ImGui found: ${ImGui_INCLUDE_DIR}")
