# @file engine/server/simulation/Module.cmake
# @brief Simulation server state and runtime sources.

set(BLITZAR_SERVER_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/server")
set(BLITZAR_SERVER_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/server")
set(BLITZAR_SERVER_INIT_SOURCE "${BLITZAR_SERVER_SOURCE_DIR}/simulation/configuration/SrvSimulationInitConfig.cpp")

set(BLITZAR_SERVER_MODULE_SOURCES
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvHelpers.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvFormatAndTheta.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/SrvCheckpoint.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/SrvIO.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/parsing/SrvBinXyz.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/parsing/SrvVtk.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvGeneration.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvGenerationContext.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvPrimitiveModels.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvPlummer.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvCosmology.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvGalaxy.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvDisk.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvScene.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvTransforms.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/state/SrvAtomic.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/configuration/SrvSimulationInitConfig.cpp"
)

set(BLITZAR_SERVER_RUNTIME_SOURCES
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/SrvControls.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvModes.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvPhysics.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvState.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/runtime/SrvExport.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/export/SrvStats.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/persistence/SrvLoadAndCheckpoint.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/SrvRebuild.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/telemetry/SrvSnapshotAndEnergy.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/telemetry/SrvPendingOps.cpp"
    "${BLITZAR_SERVER_SOURCE_DIR}/simulation/lifecycle/SrvLoop.cpp"
)
