# Refactor Map

This report is diagnostic. Potential dead files require reference, registration, export, and test review before deletion.

- Production source files: 471
- Implementation functions: 1869

## Modules

| Module | Files | Functions | Pointer candidates |
|---|---:|---:|---:|
| `apps/client-host` | 9 | 31 | 0 |
| `apps/desktop` | 4 | 8 | 0 |
| `apps/headless` | 2 | 6 | 0 |
| `apps/launcher` | 1 | 8 | 0 |
| `apps/server-service` | 3 | 11 | 0 |
| `engine/batch` | 3 | 9 | 0 |
| `engine/config` | 81 | 234 | 20 |
| `engine/core` | 2 | 1 | 3 |
| `engine/graphics` | 6 | 31 | 19 |
| `engine/physics` | 144 | 503 | 925 |
| `engine/platform` | 22 | 113 | 11 |
| `engine/server` | 42 | 181 | 111 |
| `engine/types` | 2 | 1 | 0 |
| `modules/cli` | 11 | 27 | 17 |
| `modules/echo` | 1 | 6 | 10 |
| `modules/proxy` | 3 | 22 | 18 |
| `modules/qt` | 71 | 296 | 202 |
| `runtime/client` | 28 | 229 | 40 |
| `runtime/command` | 12 | 37 | 7 |
| `runtime/ffi` | 7 | 50 | 97 |
| `runtime/protocol` | 11 | 44 | 0 |
| `runtime/server` | 6 | 21 | 6 |

## Pointer Classification

- `ABI or process boundary`: 67
- `Qt boundary`: 202
- `borrowed or ownership candidate`: 1165
- `borrowed text`: 52

## Potential Dead Files

No file in this section is deleted automatically.

- `apps/client-host/src/Cli.cpp`
- `apps/client-host/src/CliArgs.cpp`
- `apps/client-host/src/ModuleOps.cpp`
- `apps/desktop/src/PosixMain.cpp`
- `apps/desktop/src/WindowsMain.cpp`
- `apps/headless/src/main.cu`
- `engine/platform/socket/PltSocketPosix.cpp`
- `modules/cli/lifecycle/CliLifecycle.cpp`
- `modules/cli/server/CliServerOps.cpp`
- `modules/cli/state/CliState.cpp`
- `modules/proxy/support/PxySupport.cpp`
- `modules/qt/module/GuiModule.cpp`
- `modules/qt/panels/control/GuiDisclosure.cpp`
- `modules/qt/panels/control/GuiPhysics.cpp`
- `modules/qt/panels/control/GuiRender.cpp`
- `modules/qt/panels/control/GuiRun.cpp`
- `modules/qt/support/geometry/GuiViewMath.cpp`
- `modules/qt/support/performance/GuiThroughput.cpp`
- `modules/qt/support/storage/GuiLayoutStore.cpp`
- `modules/qt/support/theme/GuiTheme.cpp`
- `modules/qt/support/types/GuiEnums.cpp`
- `modules/qt/widgets/graphs/GuiGraph.cpp`
- `modules/qt/widgets/graphs/GuiPaint.cpp`
- `modules/qt/widgets/graphs/GuiSpectrumGraph.cpp`
- `modules/qt/widgets/overlays/GuiOctree.cpp`
- `modules/qt/widgets/overlays/GuiPainter.cpp`
- `modules/qt/widgets/viewport/GuiColor.cpp`
- `modules/qt/widgets/viewport/GuiGpuView.cpp`
- `modules/qt/widgets/viewport/GuiGpuViewInput.cpp`
- `modules/qt/widgets/viewport/GuiMultiView.cpp`
- `modules/qt/widgets/viewport/GuiParticle.cpp`
- `modules/qt/widgets/viewport/GuiRenderSnapshot.cpp`
- `modules/qt/widgets/viewport/GuiShaderSources.cpp`
- `modules/qt/window/actions/GuiFileActions.cpp`
- `modules/qt/window/config/GuiConfigurationEditor.cpp`
- `modules/qt/window/config/GuiWindowConfig.cpp`
- `modules/qt/window/config/GuiWindowConfigUi.cpp`
- `modules/qt/window/control/GuiController.cpp`
- `modules/qt/window/control/GuiControls.cpp`
- `modules/qt/window/control/GuiControlsPhysics.cpp`
- `modules/qt/window/control/GuiControlsRender.cpp`
- `modules/qt/window/control/GuiControlsRun.cpp`
- `modules/qt/window/control/GuiControlsScene.cpp`
- `modules/qt/window/core/GuiWidgets.cpp`
- `modules/qt/window/core/GuiWindow.cpp`
- `modules/qt/window/layout/GuiLayout.cpp`
- `modules/qt/window/layout/GuiState.cpp`
- `modules/qt/window/layout/GuiStateDefaults.cpp`
- `modules/qt/window/presentation/GuiPresenter.cpp`
- `modules/qt/window/presentation/GuiTelemetry.cpp`
- `modules/qt/window/scene/GuiSceneEditor.cpp`
- `modules/qt/window/scene/GuiSceneEditorFields.cpp`
- `modules/qt/window/scene/GuiSceneEditorProperties.cpp`
- `modules/qt/window/scene/GuiSceneEditorState.cpp`
- `modules/qt/window/workspace/GuiPersistence.cpp`
- `modules/qt/window/workspace/GuiShell.cpp`
- `runtime/client/common/CliCommon.cpp`
- `runtime/client/common/ClientCommon.cpp`
- `runtime/client/diagnostics/CliErrorBuffer.cpp`
- `runtime/client/module/CliApi.cpp`
- `runtime/client/module/CliBoundary.cpp`
- `runtime/client/module/CliHandle.cpp`
- `runtime/client/module/CliHash.cpp`
- `runtime/client/module/CliLoad.cpp`
- `runtime/client/module/CliManifest.cpp`
- `runtime/client/runtime/CliBridge.cpp`
- `runtime/client/runtime/CliBridgeState.cpp`
- `runtime/client/runtime/CliInitialState.cpp`
- `runtime/client/runtime/CliRemoteSession.cpp`
- `runtime/client/runtime/CliRuntime.cpp`
- `runtime/command/catalog/CmdCatalog.cpp`
- `runtime/command/execution/CmdBatchRunner.cpp`
- `runtime/command/execution/CmdExecutor.cpp`
- `runtime/command/parsing/CmdParser.cpp`
- `runtime/command/transport/CmdTransport.cpp`
- `runtime/ffi/core/FfiCore.cpp`
- `runtime/ffi/core/FfiOps.cpp`
- `runtime/protocol/client/PtcClient.cpp`
- `runtime/protocol/codec/parser/PtcNumber.cpp`
- `runtime/protocol/codec/parser/PtcParser.cpp`
- `runtime/protocol/codec/parser/PtcSnapshot.cpp`
- `runtime/protocol/codec/parser/PtcStatus.cpp`
- `runtime/protocol/codec/PtcJsonCodec.cpp`
- `runtime/server/core/SrvDaemon.cpp`
- `runtime/server/core/SrvDaemonCommands.cpp`
- `runtime/server/core/SrvDaemonPersistence.cpp`
- `runtime/server/core/SrvDaemonPhysics.cpp`
- `runtime/server/core/SrvDaemonTransport.cpp`

