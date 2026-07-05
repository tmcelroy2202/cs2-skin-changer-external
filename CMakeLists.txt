cmake_minimum_required(VERSION 3.20)
project(CS2SkinChanger VERSION 1.8.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "x64 target required")
endif()

# ── Dependencies ──────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ImGui REQUIRED)

# ══════════════════════════════════════════════════════════════════════════════
#  TARGET: CS2SkinChanger.exe  (standalone external overlay)
# ══════════════════════════════════════════════════════════════════════════════
add_executable(CS2SkinChanger WIN32
    src/loader/main.cpp
)

target_include_directories(CS2SkinChanger PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/MinHook/include")

# ── Sub-libraries ─────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── System libraries ──────────────────────────────────────────────────────────
if(MSVC)
    target_link_libraries(CS2SkinChanger PRIVATE
        imgui::imgui
        D3D11.lib DXGI.lib Dwmapi.lib imm32.lib Ole32.lib Windowscodecs.lib d3dcompiler.lib
    )
else()
    target_link_libraries(CS2SkinChanger PRIVATE
        imgui::imgui
        d3d11 dxgi dwmapi imm32 ole32 windowscodecs d3dcompiler gdi32
    )
endif()

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(CS2SkinChanger PRIVATE /W4 /utf-8)
    target_link_options(CS2SkinChanger PRIVATE /GUARD:CF)
else()
    target_compile_options(CS2SkinChanger PRIVATE -Wall -Wextra -DNDEBUG)
    target_link_options(CS2SkinChanger PRIVATE
        -static-libgcc -static-libstdc++ -static -lpthread -mwindows -s)
    target_link_libraries(CS2SkinChanger PRIVATE user32 kernel32)
endif()
