cmake_minimum_required(VERSION 3.20)
project(CS2SkinChanger VERSION 1.7.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "x64 target required")
endif()

# ── Build mode ─────────────────────────────────────────────────────────────────
option(CSC_BUILD_STANDALONE
    "Build as standalone overlay EXE (OFF = injected DLL)" ON)
message(STATUS "Build mode: ${CSC_BUILD_STANDALONE}"
               "  (standalone=${CSC_BUILD_STANDALONE})")

# ── Dependencies ──────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ImGui REQUIRED)

# ── Target ────────────────────────────────────────────────────────────────────
if(CSC_BUILD_STANDALONE)
    add_executable(CS2SkinChanger WIN32)
    target_compile_definitions(CS2SkinChanger PRIVATE CSC_BUILD_STANDALONE)
    if(MSVC)
        target_link_options(CS2SkinChanger PRIVATE /ENTRY:mainCRTStartup)
    endif()
else()
    add_library(CS2SkinChanger SHARED) 

    # MinHook sources (C files compiled as C by MSVC)
    target_sources(CS2SkinChanger PRIVATE
        third_party/MinHook/src/buffer.c
        third_party/MinHook/src/hook.c
        third_party/MinHook/src/trampoline.c
        third_party/MinHook/src/hde/hde64.c
    )
    set_source_files_properties(
        third_party/MinHook/src/buffer.c
        third_party/MinHook/src/hook.c
        third_party/MinHook/src/trampoline.c
        third_party/MinHook/src/hde/hde64.c
        PROPERTIES LANGUAGE C
    )
endif()


target_include_directories(CS2SkinChanger PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/MinHook/include")

# ── Sub-libraries ─────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── System libraries ──────────────────────────────────────────────────────────
# Must come after sub-libraries so MinGW ld can resolve symbols left-to-right.
if(MSVC)
    target_link_libraries(CS2SkinChanger PRIVATE
        imgui::imgui
        D3D11.lib DXGI.lib Dwmapi.lib imm32.lib Ole32.lib Windowscodecs.lib d3dcompiler.lib
    )
else()
    # MinGW — library names are lowercase (case-sensitive filesystem).
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
    target_compile_options(CS2SkinChanger PRIVATE -Wall -Wextra)
    target_link_options(CS2SkinChanger PRIVATE
        -static-libgcc -static-libstdc++ -static -lpthread)
endif()
