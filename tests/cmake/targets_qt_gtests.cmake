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
            "${BLITZAR_ROOT_DIR}/engine/server/SrvSimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/GuiGraph.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/GuiPaint.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/GuiSpectrumGraph.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/GuiController.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/GuiWindow.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/GuiWidgets.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/GuiWindowConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/GuiWindowConfigUi.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/GuiConfigurationEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorFields.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorProperties.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorState.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/GuiControls.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/actions/GuiFileActions.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/GuiLayout.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/GuiState.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/GuiStateDefaults.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/GuiPresenter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/GuiTelemetry.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/GuiPersistence.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/GuiShell.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiMultiView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiRenderSnapshot.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiGpuView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiGpuViewInput.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiShaderSources.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/GuiOctree.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/GuiPainter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiParticle.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/GuiColor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/types/GuiEnums.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/performance/GuiThroughput.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/theme/GuiTheme.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/geometry/GuiViewMath.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/storage/GuiLayoutStore.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/GuiPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/GuiDisclosure.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/GuiRender.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/GuiRun.cpp"
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
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorFields.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorProperties.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/scene/GuiSceneEditorState.cpp"
        LIBS
            Qt6::Widgets
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
    BLITZAR_configure_qt_runtime_deploy(blitzarQtSceneEditorGTests)
endif()
