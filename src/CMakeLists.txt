# ── Core library ──────────────────────────────────────────────────────────────
add_library(csc_core STATIC
    core/Logging.cpp
    core/Module.cpp
    core/Memory.cpp
    core/PatternScanner.cpp
    core/InterfaceDiscovery.cpp
    core/OffsetManager.cpp
)
target_include_directories(csc_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# ── UI library ────────────────────────────────────────────────────────────────
add_library(csc_ui STATIC
    ui/Framework.cpp
    ui/Panels.cpp
    ui/overlay/OverlayWindow.cpp
    ui/panels/SkinChangerPanel.cpp
    ui/panels/ConsolePanel.cpp
)
target_include_directories(csc_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(csc_ui PUBLIC imgui::imgui)

# ── Main target linkage ───────────────────────────────────────────────────────
target_sources(libCS2SkinChanger PRIVATE
    main.cpp
    cs2/SkinChanger.cpp
)

if(MSVC)
    target_sources(libCS2SkinChanger PRIVATE resources/version.rc)
endif()

target_link_libraries(libCS2SkinChanger PRIVATE
    csc_core
    csc_ui
)
