cmake_minimum_required(VERSION 3.20)
project(S2ResearchFramework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "x64 target required")
endif()

# ── Build mode ─────────────────────────────────────────────────────────────────
option(S2RF_BUILD_STANDALONE
    "Build as standalone overlay EXE (OFF = injected DLL)" OFF)

message(STATUS "Build mode: ${S2RF_BUILD_STANDALONE}"
               "  (standalone=${S2RF_BUILD_STANDALONE})")

# ── Dependencies ──────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ImGui REQUIRED)

# ── Target ────────────────────────────────────────────────────────────────────
if(S2RF_BUILD_STANDALONE)
    add_executable(S2ResearchFramework WIN32)
    target_compile_definitions(S2ResearchFramework PRIVATE S2RF_BUILD_STANDALONE)
else()
    add_library(S2ResearchFramework SHARED)
endif()

target_include_directories(S2ResearchFramework PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ── System libraries ──────────────────────────────────────────────────────────
target_link_libraries(S2ResearchFramework PRIVATE
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
    target_compile_options(S2ResearchFramework PRIVATE /W4 /utf-8)
    target_link_options(S2ResearchFramework PRIVATE /GUARD:CF)
else()
    target_compile_options(S2ResearchFramework PRIVATE -Wall -Wextra)
endif()
