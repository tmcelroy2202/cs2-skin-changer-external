cmake_minimum_required(VERSION 3.20)
project(veneer-linux-test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Paths ───────────────────────────────────────────────────────────────────
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(IMGUI_DIR "${SRC_DIR}/third_party/imgui")
set(GLFW_DIR "/tmp/glfw")
set(GL3W_DIR "/tmp/gl3w-out")

# ── gl3w OpenGL loader ──────────────────────────────────────────────────────
add_library(gl3w STATIC "${GL3W_DIR}/src/gl3w.c")
target_include_directories(gl3w PUBLIC "${GL3W_DIR}/include")

# ── ImGui with GLFW + OpenGL3 backends ──────────────────────────────────────
file(GLOB IMGUI_SRCS
    "${IMGUI_DIR}/*.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
)
add_library(imgui_core STATIC ${IMGUI_SRCS})
target_include_directories(imgui_core PUBLIC "${IMGUI_DIR}")
target_include_directories(imgui_core PUBLIC "${IMGUI_DIR}/backends")
target_include_directories(imgui_core PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(imgui_core PRIVATE
    IMGUI_IMPL_API=
    IMGUI_IMPL_OPENGL_LOADER_GL3W
)

# ── Test executable ─────────────────────────────────────────────────────────
add_executable(veneer-test
    main.cpp
    "${SRC_DIR}/src/ui/panels/SkinChangerPanel.cpp"
)

target_include_directories(veneer-test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/stubs"   # Override cs2/SkinChanger.h etc.
    "${SRC_DIR}/src"                      # Real source headers
    "${IMGUI_DIR}"
    "${GLFW_DIR}/include"
    "${GL3W_DIR}/include"
)

target_link_libraries(veneer-test PRIVATE
    imgui_core
    gl3w
    "${GLFW_DIR}/build/src/libglfw3.a"
    dl
    X11
    pthread
    GL
)
