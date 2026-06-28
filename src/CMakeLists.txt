# ── Core library ──────────────────────────────────────────────────────────────
add_library(s2rf_core STATIC
    core/Logging.cpp
    core/Module.cpp
    core/Memory.cpp
    core/PatternScanner.cpp
    core/InterfaceDiscovery.cpp
    core/OffsetManager.cpp
)
target_include_directories(s2rf_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(s2rf_core PUBLIC
    $<$<BOOL:${spdlog_FOUND}>:spdlog::spdlog>
)

# ── Engine library ────────────────────────────────────────────────────────────
add_library(s2rf_engine STATIC
    engine/Interfaces.cpp
    engine/schema/SchemaSystem.cpp
    engine/schema/SchemaReflection.cpp
    engine/entity/EntitySystem.cpp
    engine/resource/ResourceSystem.cpp
    engine/render/RenderSystem.cpp
    engine/render/MaterialSystem.cpp
)
target_include_directories(s2rf_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(s2rf_engine PUBLIC s2rf_core)

# ── UI library ────────────────────────────────────────────────────────────────
add_library(s2rf_ui STATIC
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
target_include_directories(s2rf_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(s2rf_ui PUBLIC s2rf_engine imgui::imgui)

# ── Analysis library ──────────────────────────────────────────────────────────
add_library(s2rf_analysis STATIC
    analysis/ObjectInspector.cpp
    analysis/AssetAnalyzer.cpp
)
target_include_directories(s2rf_analysis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(s2rf_analysis PUBLIC s2rf_engine)

# ── Main target linkage ───────────────────────────────────────────────────────
target_sources(S2ResearchFramework PRIVATE main.cpp)
target_link_libraries(S2ResearchFramework PRIVATE
    s2rf_core
    s2rf_engine
    s2rf_ui
    s2rf_analysis
)
