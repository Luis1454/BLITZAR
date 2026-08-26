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
    src/particles/buffers/ParticleBuffer.cpp
    src/particles/buffers/AccelerationBuffer.cpp
    src/particles/buffers/ParticleAccess.cpp
    src/particles/source/SourceBuffer.cpp
    src/physics/gravity/GravityLaw.cpp
    src/parallel/mpi/collectives/MpiCollectives.cpp
    src/parallel/mpi/collectives/MpiBroadcast.cpp
    src/parallel/mpi/domain/DomainBounds.cpp
    src/parallel/mpi/domain/DomainDecomposition.cpp
    src/parallel/mpi/domain/DomainInit.cpp
    src/parallel/mpi/domain/DomainQuery.cpp
    src/parallel/mpi/domain/DomainValidate.cpp
    src/parallel/mpi/domain/DomainPartition.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostTransport.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostBegin.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostLayout.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostPost.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostComplete.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostReceive.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostStorage.cpp
    src/parallel/mpi/exchange/ghost/MpiGhostState.cpp
    src/parallel/mpi/gather/MpiGather.cpp
    src/parallel/mpi/gather/MpiGatherLocal.cpp
    src/parallel/mpi/gather/MpiGatherPlan.cpp
    src/parallel/mpi/gather/MpiGatherData.cpp
    src/parallel/mpi/context/MpiContext.cpp
    src/parallel/mpi/context/MpiContextCapacity.cpp
    src/parallel/mpi/context/MpiContextCollectives.cpp
    src/parallel/mpi/context/MpiContextGhost.cpp
    src/parallel/mpi/context/MpiContextPackets.cpp
    src/parallel/mpi/native/MpiStatus.cpp
    src/parallel/mpi/exchange/MpiExchange.cpp
    src/parallel/mpi/exchange/migration/MpiMigration.cpp
    src/parallel/mpi/exchange/packets/MpiPacketTransport.cpp
    src/parallel/mpi/exchange/packets/MpiPacketPrepare.cpp
    src/parallel/mpi/exchange/packets/MpiPacketCounts.cpp
    src/parallel/mpi/exchange/packets/MpiPacketAllToAll.cpp
    src/parallel/mpi/exchange/packets/MpiPacketAllGather.cpp
    src/parallel/mpi/native/MpiNative.cpp
    src/parallel/mpi/native/MpiNativeCollectives.cpp
    src/parallel/mpi/native/MpiNativePackets.cpp
    src/parallel/mpi/native/MpiNativeGhost.cpp
    src/parallel/mpi/native/MpiSession.cpp
    src/parallel/mpi/exchange/packets/PacketWire.cpp
    src/sdk/c/Api.cpp
    src/sdk/c/ApiSupport.cpp
    src/sdk/c/ApiInfo.cpp
    src/sdk/c/ApiConfig.cpp
    src/sdk/c/ApiParticles.cpp
    src/sdk/c/ApiStep.cpp
    src/sdk/cpp/CppContext.cpp
    src/sdk/cpp/CppSimulation.cpp
    src/sdk/cpp/CppSimulationConfig.cpp
    src/sdk/cpp/CppSimulationData.cpp
    src/simulation/facade/Simulation.cpp
    src/simulation/composition/SimulationResources.cpp
    src/simulation/storage/ParticleStorage.cpp
    src/simulation/configuration/SimulationConfig.cpp
    src/simulation/input/ParticleSet.cpp
    src/simulation/input/ParticleGet.cpp
    src/simulation/input/ParticleInputStage.cpp
    src/simulation/step/packets/Packets.cpp
    src/simulation/transaction/Snapshots.cpp
    src/simulation/step/Step.cpp
    src/simulation/step/preparation/StepPrepare.cpp
    src/simulation/step/migration/StepMigration.cpp
    src/simulation/transaction/Transaction.cpp
    src/solvers/barnes_hut/BarnesHutSolver.cpp
    src/solvers/barnes_hut/tree/BarnesTree.cpp
    src/solvers/threading/ThreadStackPool.cpp
    src/solvers/direct/DirectSolver.cpp
    src/solvers/direct/compute/DirectCompute.cpp
    src/solvers/direct/force/DirectForce.cpp
    src/solvers/fmm/FmmSolver.cpp
    src/solvers/fmm/multipole/FmmMultipole.cpp
    src/solvers/fmm/traversal/FmmTraverse.cpp
    src/trees/octree/Octree.cpp
    src/trees/octree/access/OctAccess.cpp
    src/trees/octree/construction/OctConstruction.cpp
    src/trees/octree/ordering/OctMorton.cpp
    src/trees/octree/properties/OctProperties.cpp
)
target_sources(blitzar PRIVATE ${BLITZAR_LIBRARY_SOURCES})

if(BLITZAR_HIP_ENABLED)
    set(BLITZAR_HIP_SOURCES
        src/accelerators/gpu/hip/memory/Buffers.hip
        src/accelerators/gpu/hip/runtime/Context.hip
        src/accelerators/gpu/hip/direct/DirectKernel.hip
        src/accelerators/gpu/hip/barnes_hut/BarnesHutKernel.hip
    )
    if(BLITZAR_HIP_LANGUAGE STREQUAL "CUDA")
        set_source_files_properties(${BLITZAR_HIP_SOURCES} PROPERTIES
            LANGUAGE CUDA
        )
    endif()
    target_sources(blitzar PRIVATE ${BLITZAR_HIP_SOURCES})
else()
    target_sources(blitzar PRIVATE
        src/accelerators/gpu/hip/runtime/Fallback.cpp
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
