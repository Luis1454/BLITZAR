# @file tests/cmake/targets_qt_gtests.cmake
# @brief Qt integration and scene editor test targets.

if(TARGET Qt6::Widgets AND TARGET Qt6::OpenGLWidgets AND TARGET blitzarRustRuntime AND BLITZAR_TEST_INT_UI_SOURCES)
    BLITZAR_add_gtest(blitzarQtMainWindowGTests
        LABELS ui_integration integration_real
        TIMEOUT 45
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_UI_SOURCES}
            ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
            "${BLITZAR_ROOT_DIR}/tests/support/qt_test_utils.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/Graph.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/Paint.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/SpectrumGraph.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/Controller.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/Window.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/Widgets.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/WindowConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/WindowConfigUi.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/ConfigurationEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorFields.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorProperties.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorState.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/Controls.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/actions/FileActions.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/Layout.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/State.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/StateDefaults.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/Presenter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/Telemetry.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/Persistence.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/Shell.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/MultiView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/RenderSnapshot.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GpuView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GpuViewInput.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/ShaderSources.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/Octree.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/Painter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/Particle.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/Color.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/types/Enums.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/performance/Throughput.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/theme/Theme.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/geometry/ViewMath.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/storage/LayoutStore.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Physics.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Disclosure.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Render.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Run.cpp"
            ${BLITZAR_GRAPHICS_SOURCES}
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            Qt6::Widgets
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
    BLITZAR_configure_qt_runtime_deploy(blitzarQtMainWindowGTests)
endif()

if(TARGET Qt6::Widgets AND BLITZAR_BUILD_TESTS)
    BLITZAR_add_gtest(blitzarQtSceneEditorGTests
        LABELS ui_integration gui_non_intrusive
        TIMEOUT 20
        SOURCES
            "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_scene_editor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorFields.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorProperties.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/SceneEditorState.cpp"
        LIBS
            Qt6::Widgets
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
    BLITZAR_configure_qt_runtime_deploy(blitzarQtSceneEditorGTests)
endif()
