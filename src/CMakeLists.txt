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

# ── Engine library ────────────────────────────────────────────────────────────
add_library(csc_engine STATIC
    engine/Interfaces.cpp
    engine/schema/SchemaSystem.cpp
    engine/schema/SchemaReflection.cpp
    engine/entity/EntitySystem.cpp
    engine/resource/ResourceSystem.cpp
    engine/render/RenderSystem.cpp
    engine/render/MaterialSystem.cpp
)
target_include_directories(csc_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(csc_engine PUBLIC csc_core)

# ── UI library ────────────────────────────────────────────────────────────────
add_library(csc_ui STATIC
    ui/Framework.cpp
    ui/Panels.cpp
    ui/overlay/OverlayWindow.cpp
    ui/panels/SchemaBrowserPanel.cpp
    ui/panels/EntityBrowserPanel.cpp
    ui/panels/ResourceViewerPanel.cpp
    ui/panels/MemoryInspectorPanel.cpp
    ui/panels/RenderDebugPanel.cpp
    ui/panels/ConsolePanel.cpp
    ui/panels/SkinChangerPanel.cpp
    ui/widgets/HexViewWidget.cpp
    ui/widgets/PropertyGridWidget.cpp
)
target_include_directories(csc_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(csc_ui PUBLIC csc_engine imgui::imgui)

# ── Analysis library ──────────────────────────────────────────────────────────
add_library(csc_analysis STATIC
    analysis/ObjectInspector.cpp
    analysis/AssetAnalyzer.cpp
)
target_include_directories(csc_analysis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(csc_analysis PUBLIC csc_engine)

# ── Main target linkage ───────────────────────────────────────────────────────
target_sources(CS2SkinChanger PRIVATE main.cpp)

if(MSVC)
    # Version resource (.rc) + custom manifest for proper metadata
    target_sources(CS2SkinChanger PRIVATE resources/version.rc)
    target_link_options(CS2SkinChanger PRIVATE
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/resources/app.manifest"
        "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
endif()

target_link_libraries(CS2SkinChanger PRIVATE
    csc_core
    csc_engine
    csc_ui
    csc_analysis
)
