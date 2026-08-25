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
    src/integration/KdkCheckpoint.cpp
    src/particles/ParticleArena.cpp
    src/particles/ParticleBuffer.cpp
    src/particles/AccelerationBuffer.cpp
    src/particles/ParticleAccess.cpp
    src/particles/SourceBuffer.cpp
    src/physics/GravityLaw.cpp
    src/parallel/MpiCollectives.cpp
    src/parallel/MpiBroadcast.cpp
    src/parallel/Bounds.cpp
    src/parallel/DomainDecomposition.cpp
    src/parallel/DomainInit.cpp
    src/parallel/DomainQuery.cpp
    src/parallel/DomainValidate.cpp
    src/parallel/Partition.cpp
    src/parallel/MpiGhostTransport.cpp
    src/parallel/MpiGhostBegin.cpp
    src/parallel/MpiGhostLayout.cpp
    src/parallel/MpiGhostPost.cpp
    src/parallel/MpiGhostComplete.cpp
    src/parallel/MpiGhostReceive.cpp
    src/parallel/MpiGhostStorage.cpp
    src/parallel/MpiGhostState.cpp
    src/parallel/MpiGather.cpp
    src/parallel/MpiGatherLocal.cpp
    src/parallel/MpiGatherPlan.cpp
    src/parallel/MpiGatherData.cpp
    src/parallel/MpiContext.cpp
    src/parallel/MpiContextCapacity.cpp
    src/parallel/MpiContextCollectives.cpp
    src/parallel/MpiContextGhost.cpp
    src/parallel/MpiContextPackets.cpp
    src/parallel/MpiStatus.cpp
    src/parallel/MpiExchange.cpp
    src/parallel/MpiMigration.cpp
    src/parallel/MpiPacketTransport.cpp
    src/parallel/MpiPacketPrepare.cpp
    src/parallel/MpiPacketCounts.cpp
    src/parallel/MpiPacketAllToAll.cpp
    src/parallel/MpiPacketAllGather.cpp
    src/parallel/MpiNative.cpp
    src/parallel/MpiNativeCollectives.cpp
    src/parallel/MpiNativePackets.cpp
    src/parallel/MpiNativeGhost.cpp
    src/parallel/MpiSession.cpp
    src/parallel/MpiTypes.cpp
    src/sdk/Api.cpp
    src/sdk/ApiSupport.cpp
    src/sdk/ApiInfo.cpp
    src/sdk/ApiConfig.cpp
    src/sdk/ApiParticles.cpp
    src/sdk/ApiStep.cpp
    src/sdk/CppContext.cpp
    src/sdk/CppSimulation.cpp
    src/sdk/CppSimulationConfig.cpp
    src/sdk/CppSimulationData.cpp
    src/sdk/Simulation.cpp
    src/sdk/SimulationRuntime.cpp
    src/sdk/ParticleStorage.cpp
    src/sdk/Config.cpp
    src/sdk/ParticleSet.cpp
    src/sdk/ParticleGet.cpp
    src/sdk/ParticleInputStage.cpp
    src/sdk/Packets.cpp
    src/sdk/Snapshots.cpp
    src/sdk/Step.cpp
    src/sdk/StepPrepare.cpp
    src/sdk/StepMigration.cpp
    src/sdk/Transaction.cpp
    src/solvers/barnes_hut/BarnesHutSolver.cpp
    src/solvers/barnes_hut/Tree.cpp
    src/solvers/barnes_hut/ThreadStackPool.cpp
    src/solvers/direct/DirectSolver.cpp
    src/solvers/direct/DirectCompute.cpp
    src/solvers/direct/DirectForce.cpp
    src/solvers/fmm/FmmSolver.cpp
    src/solvers/fmm/FmmMultipole.cpp
    src/solvers/fmm/FmmTraverse.cpp
    src/trees/Octree.cpp
    src/trees/OctAccess.cpp
    src/trees/OctBuild.cpp
    src/trees/OctMorton.cpp
    src/trees/OctProperties.cpp
)
target_sources(blitzar PRIVATE ${BLITZAR_LIBRARY_SOURCES})

if(BLITZAR_HIP_ENABLED)
    set(BLITZAR_HIP_SOURCES
        src/gpu/HipBuffers.hip
        src/gpu/HipContext.hip
        src/solvers/gpu/DirectKernel.hip
        src/solvers/gpu/BarnesHutKernel.hip
    )
    if(BLITZAR_HIP_LANGUAGE STREQUAL "CUDA")
        set_source_files_properties(${BLITZAR_HIP_SOURCES} PROPERTIES
            LANGUAGE CUDA
        )
    endif()
    target_sources(blitzar PRIVATE ${BLITZAR_HIP_SOURCES})
else()
    target_sources(blitzar PRIVATE
        src/gpu/HipContextStub.cpp
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
