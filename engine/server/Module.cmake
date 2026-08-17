# @file engine/server/Module.cmake
# @brief Simulation server state and runtime sources.

set(BLITZAR_SERVER_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/server/include")
set(BLITZAR_SERVER_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/server/src")
set(BLITZAR_SERVER_INIT_SOURCE "${BLITZAR_SERVER_SOURCE_DIR}/SimulationInitConfig.cpp")

set(BLITZAR_SERVER_MODULE_SOURCES
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/core/Helpers.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/core/FormatAndTheta.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/Checkpoint.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/IO.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/parsing/BinXyz.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/parsing/Vtk.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Generation.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/GenerationContext.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/PrimitiveModels.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Plummer.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Cosmology.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Galaxy.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Disk.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Scene.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Transforms.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/Atomic.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/SimulationInitConfig.cpp"
)

set(BLITZAR_SERVER_RUNTIME_SOURCES
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/Controls.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/Modes.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/Physics.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvState.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/Export.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/export/Stats.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/LoadAndCheckpoint.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/Rebuild.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/telemetry/SnapshotAndEnergy.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/telemetry/PendingOps.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/Loop.cpp"
)
