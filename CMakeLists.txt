cmake_minimum_required(VERSION 3.20)
project(CS2SkinChanger VERSION 1.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "x64 target required")
endif()

# ── Build mode ─────────────────────────────────────────────────────────────────
option(CSC_BUILD_STANDALONE
    "Build as standalone overlay EXE (OFF = injected DLL)" OFF)

message(STATUS "Build mode: ${CSC_BUILD_STANDALONE}"
               "  (standalone=${CSC_BUILD_STANDALONE})")

# ── Dependencies ──────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ImGui REQUIRED)

# ── Target ────────────────────────────────────────────────────────────────────
if(CSC_BUILD_STANDALONE)
    add_executable(CS2SkinChanger WIN32)
    target_compile_definitions(CS2SkinChanger PRIVATE CSC_BUILD_STANDALONE)
    target_link_options(CS2SkinChanger PRIVATE /ENTRY:mainCRTStartup)
else()
    add_library(CS2SkinChanger SHARED)
endif()

target_include_directories(CS2SkinChanger PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ── System libraries ──────────────────────────────────────────────────────────
target_link_libraries(CS2SkinChanger PRIVATE
    D3D11.lib   # Direct3D 11
    DXGI.lib    # DXGI swap chain
    Dwmapi.lib  # Desktop Window Manager (overlay transparency)
    imm32.lib   # IME support (ImGui)
    imgui::imgui
)

# ── Sub-libraries ─────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(CS2SkinChanger PRIVATE /W4 /utf-8)
    target_link_options(CS2SkinChanger PRIVATE /GUARD:CF)
else()
    target_compile_options(CS2SkinChanger PRIVATE -Wall -Wextra)
endif()
