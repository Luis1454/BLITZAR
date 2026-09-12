if(BLITZAR_BUILD_SHARED)
    add_library(blitzar SHARED)
    target_compile_definitions(blitzar
        PRIVATE BLITZAR_BUILDING
        PUBLIC BLITZAR_SHARED
    )
else()
    add_library(blitzar STATIC)
endif()

set(BLITZAR_LIBRARY_SOURCES
    src/integration/kdk/KdkCheckpoint.cpp
    src/particles/arena/ParticleArena.cpp
    src/particles/buffer/ParticleBuffer.cpp
    src/particles/buffer/ParticleAccelerationBuffer.cpp
    src/particles/buffer/ParticleAccess.cpp
    src/particles/source/ParticleSourceBuffer.cpp
    src/physics/gravity/GravityLaw.cpp
    src/physics/conservation/ConservationMetrics.cpp
    src/physics/reduction/ScalarReduction.cpp
    src/mpi/collectives/MpiCollectives.cpp
    src/mpi/collectives/MpiBroadcast.cpp
    src/mpi/domain/MpiDomainBounds.cpp
    src/mpi/domain/MpiDomainDecomposition.cpp
    src/mpi/domain/MpiDomainInit.cpp
    src/mpi/domain/MpiDomainQuery.cpp
    src/mpi/domain/MpiDomainValidate.cpp
    src/mpi/domain/MpiDomainPartition.cpp
    src/mpi/ghost/MpiGhostTransport.cpp
    src/mpi/ghost/MpiGhostBegin.cpp
    src/mpi/ghost/MpiGhostLayout.cpp
    src/mpi/ghost/MpiGhostPost.cpp
    src/mpi/ghost/MpiGhostComplete.cpp
    src/mpi/ghost/MpiGhostReceive.cpp
    src/mpi/ghost/MpiGhostStorage.cpp
    src/mpi/ghost/MpiGhostState.cpp
    src/mpi/gather/MpiGather.cpp
    src/mpi/gather/MpiGatherLocal.cpp
    src/mpi/gather/MpiGatherPlan.cpp
    src/mpi/gather/MpiGatherData.cpp
    src/mpi/runtime/MpiContext.cpp
    src/mpi/runtime/MpiCapacity.cpp
    src/mpi/runtime/MpiCollectiveBridge.cpp
    src/mpi/runtime/MpiGhost.cpp
    src/mpi/runtime/MpiPackets.cpp
    src/mpi/native/MpiNativeStatus.cpp
    src/mpi/exchange/MpiExchange.cpp
    src/mpi/exchange/MpiMigration.cpp
    src/mpi/packets/MpiPacketTransport.cpp
    src/mpi/packets/MpiPacketPrepare.cpp
    src/mpi/packets/MpiPacketCounts.cpp
    src/mpi/packets/MpiPacketAllToAll.cpp
    src/mpi/packets/MpiPacketAllGather.cpp
    src/mpi/native/MpiNative.cpp
    src/mpi/native/MpiNativeCollectives.cpp
    src/mpi/native/MpiNativePackets.cpp
    src/mpi/native/MpiNativeGhost.cpp
    src/mpi/native/MpiNativeSession.cpp
    src/mpi/packets/MpiPacketWire.cpp
    src/sdk/c/CApi.cpp
    src/sdk/c/CApiSupport.cpp
    src/sdk/c/CApiInfo.cpp
    src/sdk/c/CApiConfig.cpp
    src/sdk/c/CApiParticles.cpp
    src/sdk/c/CApiStep.cpp
    src/sdk/cpp/CppContext.cpp
    src/sdk/cpp/CppSimulation.cpp
    src/sdk/cpp/CppSimulationConfig.cpp
    src/sdk/cpp/CppSimulationData.cpp
    src/sdk/cpp/CppSimulationAccess.cpp
    src/io/snap/codec/SnapshotWire.cpp
    src/io/snap/codec/SnapshotChecksum.cpp
    src/io/snap/delta/SnapshotDelta.cpp
    src/io/snap/delta/SnapshotDeltaPayload.cpp
    src/io/snap/delta/SnapshotDeltaStream.cpp
    src/io/snap/codec/SnapshotWriter.cpp
    src/io/snap/codec/SnapshotReader.cpp
    src/io/hdf5/Hdf5Writer.cpp
    src/io/hdf5/Hdf5Reader.cpp
    src/io/md/manifest/MetadataManifest.cpp
    src/io/md/manifest/MetadataManifestJson.cpp
    src/io/md/cursor/MetadataCursor.cpp
    src/io/md/fields/MetadataFields.cpp
    src/io/md/reader/MetadataReader.cpp
    src/io/md/reader/MetadataReaderJson.cpp
    src/io/md/run/MetadataRun.cpp
    src/io/md/text/MetadataText.cpp
    src/io/diag/ConservationCsv.cpp
    src/io/post/PostProcess.cpp
    src/io/post/output/PostProcessOutput.cpp
    src/io/post/state/PostProcessState.cpp
    src/io/post/input/PostProcessInput.cpp
    src/simulation/Sim.cpp
    src/simulation/runtime/SimRuntime.cpp
    src/simulation/state/SimParticleState.cpp
    src/simulation/SimConfig.cpp
    src/simulation/cfg/SimConfigFile.cpp
    src/simulation/cfg/SimConfigValue.cpp
    src/simulation/cfg/simulation/SimConfigSimulation.cpp
    src/simulation/cfg/physics/SimConfigPhysics.cpp
    src/simulation/cfg/output/SimConfigOutput.cpp
    src/simulation/cfg/diagnostics/SimConfigDiagnostics.cpp
    src/simulation/cfg/restart/SimConfigRestart.cpp
    src/simulation/cfg/execution/SimConfigExecution.cpp
    src/simulation/cfg/run/SimConfigRun.cpp
    src/simulation/init/SimConfigState.cpp
    src/simulation/state/SimParticleSet.cpp
    src/simulation/state/SimParticleGet.cpp
    src/simulation/stage/SimParticleStage.cpp
    src/simulation/step/packets/SimStepPackets.cpp
    src/simulation/tx/SimSnapshots.cpp
    src/simulation/step/SimStep.cpp
    src/simulation/step/prepare/SimStepPrepare.cpp
    src/simulation/step/migration/SimStepMigration.cpp
    src/simulation/tx/SimTransaction.cpp
    src/solvers/bh/BhSolver.cpp
    src/solvers/bh/BhTree.cpp
    src/solvers/threading/ThreadStackPool.cpp
    src/solvers/direct/DirectSolver.cpp
    src/solvers/direct/DirectCompute.cpp
    src/solvers/direct/DirectForce.cpp
    src/solvers/fmm/FmmSolver.cpp
    src/solvers/fmm/FmmMultipole.cpp
    src/solvers/fmm/FmmTraverse.cpp
    src/solvers/fmm/kifmm/KifmmWorkspace.cpp
    src/solvers/fmm/kifmm/KifmmSolver.cpp
    src/solvers/fmm/kifmm/KifmmOperator.cpp
    src/solvers/fmm/kifmm/KifmmTraverse.cpp
    src/trees/octree/Octree.cpp
    src/trees/octree/view/OctreeAccess.cpp
    src/trees/octree/build/OctreeConstruction.cpp
    src/trees/octree/build/OctreeMorton.cpp
    src/trees/octree/view/OctreeProperties.cpp
    src/trees/octree/resource/OctreeResource.cpp
    src/trees/octree/view/OctreeView.cpp
    src/solvers/SolverTreeResources.cpp
    src/grid/layout/GridLayout.cpp
    src/grid/resource/GridResource.cpp
    src/solvers/pm/PmSolver.cpp
    src/solvers/pm/PmSolverForce.cpp
    src/solvers/treepm/TreePmSolver.cpp
    src/solvers/treepm/TreePmSolverForce.cpp
)
target_sources(blitzar PRIVATE ${BLITZAR_LIBRARY_SOURCES})

if(BLITZAR_HIP_ENABLED)
    set(BLITZAR_HIP_SOURCES
        src/gpu/memory/GpuBuffers.hip
        src/gpu/runtime/GpuContext.hip
        src/gpu/direct/GpuDirectKernel.hip
        src/gpu/bh/GpuBhKernel.hip
    )
    if(BLITZAR_HIP_LANGUAGE STREQUAL "CUDA")
        set_source_files_properties(${BLITZAR_HIP_SOURCES} PROPERTIES
            LANGUAGE CUDA
        )
    endif()
    target_sources(blitzar PRIVATE ${BLITZAR_HIP_SOURCES})
else()
    target_sources(blitzar PRIVATE
        src/gpu/runtime/GpuFallback.cpp
    )
endif()

target_compile_features(blitzar PUBLIC cxx_std_20)
target_compile_definitions(blitzar PRIVATE
    BLITZAR_BUILD_PRODUCT_VERSION="${PROJECT_VERSION}"
    BLITZAR_BUILD_PLAN_VERSION="${BLITZAR_PLAN_VERSION}"
)
target_include_directories(blitzar
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

blitzar_enable_warnings(blitzar)
blitzar_enable_hip(blitzar)
blitzar_enable_mpi(blitzar)
blitzar_enable_hdf5(blitzar)

if(BLITZAR_ENABLE_OPENMP)
    target_link_libraries(blitzar PUBLIC OpenMP::OpenMP_CXX)
endif()

if(BLITZAR_ENABLE_SANITIZERS)
    target_compile_options(blitzar PUBLIC
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(blitzar PUBLIC -fsanitize=address,undefined)
endif()
