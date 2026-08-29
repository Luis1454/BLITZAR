add_library(blitzar_public_header_compile OBJECT
    tests/contracts/ContractCPublicTest.c
    tests/contracts/ContractCppPublicTest.cpp
)
target_include_directories(blitzar_public_header_compile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(blitzar_public_header_compile PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_public_header_compile)

add_executable(blitzar_lifecycle_test
    tests/simulation/SimLifecycleTest.cpp
)
target_link_libraries(blitzar_lifecycle_test PRIVATE blitzar)
target_link_libraries(blitzar_lifecycle_test PRIVATE Threads::Threads)
target_compile_features(blitzar_lifecycle_test PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_lifecycle_test)

add_executable(blitzar_config_test
    tests/simulation/SimConfigFileTest.cpp
)
target_link_libraries(blitzar_config_test PRIVATE blitzar)
target_compile_features(blitzar_config_test PRIVATE cxx_std_20)
target_include_directories(blitzar_config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_config_test)

add_executable(blitzar_config_run_test
    tests/simulation/SimConfigRunTest.cpp
)
target_link_libraries(blitzar_config_run_test PRIVATE blitzar)
target_compile_features(blitzar_config_run_test PRIVATE cxx_std_20)
target_include_directories(blitzar_config_run_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_config_run_test)

add_executable(blitzar_c_api_test
    tests/contracts/ContractCApiTest.c
)
target_link_libraries(blitzar_c_api_test PRIVATE blitzar)
set_property(TARGET blitzar_c_api_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_c_api_test)

add_executable(blitzar_contract_test
    tests/contracts/ContractTest.cpp
)
target_link_libraries(blitzar_contract_test PRIVATE blitzar)
target_compile_features(blitzar_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_contract_test)

add_executable(blitzar_snapshot_contract_test
    tests/contracts/ContractSnapshotTest.cpp
)
target_link_libraries(blitzar_snapshot_contract_test PRIVATE blitzar)
target_compile_features(blitzar_snapshot_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_snapshot_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_snapshot_contract_test)

add_executable(blitzar_snapshot_io_test
    tests/io/IoSnapshotTest.cpp
)
target_link_libraries(blitzar_snapshot_io_test PRIVATE blitzar)
target_compile_features(blitzar_snapshot_io_test PRIVATE cxx_std_20)
target_include_directories(blitzar_snapshot_io_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_snapshot_io_test)

add_executable(blitzar_metadata_io_test
    tests/io/IoMetadataTest.cpp
)
target_link_libraries(blitzar_metadata_io_test PRIVATE blitzar)
target_compile_features(blitzar_metadata_io_test PRIVATE cxx_std_20)
target_include_directories(blitzar_metadata_io_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_metadata_io_test)

add_executable(blitzar_cli_output_test
    apps/blitzar/BlitzarOutput.cpp
    apps/blitzar/BlitzarRestart.cpp
    apps/blitzar/BlitzarSummary.cpp
    apps/blitzar/BlitzarRun.cpp
    tests/io/IoCliOutputTest.cpp
)
target_link_libraries(blitzar_cli_output_test PRIVATE blitzar)
target_compile_features(blitzar_cli_output_test PRIVATE cxx_std_20)
target_include_directories(blitzar_cli_output_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/blitzar
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_cli_output_test)

add_executable(blitzar_cli_summary_test
    apps/blitzar/BlitzarOutput.cpp
    apps/blitzar/BlitzarRestart.cpp
    apps/blitzar/BlitzarSummary.cpp
    apps/blitzar/BlitzarRun.cpp
    tests/io/IoCliSummaryTest.cpp
)
target_link_libraries(blitzar_cli_summary_test PRIVATE blitzar)
target_compile_features(blitzar_cli_summary_test PRIVATE cxx_std_20)
target_include_directories(blitzar_cli_summary_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/blitzar
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_cli_summary_test)

add_executable(blitzar_cli_restart_test
    apps/blitzar/BlitzarOutput.cpp
    apps/blitzar/BlitzarRestart.cpp
    apps/blitzar/BlitzarSummary.cpp
    apps/blitzar/BlitzarRun.cpp
    tests/fixtures/FixtureRestart.cpp
    tests/io/IoCliRestartTest.cpp
)
target_link_libraries(blitzar_cli_restart_test PRIVATE blitzar)
target_compile_features(blitzar_cli_restart_test PRIVATE cxx_std_20)
target_include_directories(blitzar_cli_restart_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/blitzar
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_cli_restart_test)

if(BLITZAR_BUILD_CLI)
    add_executable(blitzar_cli_process_restart_test
        tests/fixtures/FixtureProcess.cpp
        tests/fixtures/FixtureRestart.cpp
        tests/io/IoCliProcessRestartTest.cpp
    )
    target_link_libraries(blitzar_cli_process_restart_test PRIVATE blitzar)
    target_compile_features(blitzar_cli_process_restart_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_cli_process_restart_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_cli_process_restart_test)

    add_executable(blitzar_cli_postprocess_test
        apps/blitzar/BlitzarOutput.cpp
        apps/blitzar/BlitzarPostProcess.cpp
        apps/blitzar/BlitzarRestart.cpp
        apps/blitzar/BlitzarRun.cpp
        apps/blitzar/BlitzarSummary.cpp
        tests/fixtures/FixtureProcess.cpp
        tests/fixtures/FixtureRestart.cpp
        tests/io/IoCliPostProcessTest.cpp
    )
    target_link_libraries(blitzar_cli_postprocess_test PRIVATE blitzar)
    target_compile_features(blitzar_cli_postprocess_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_cli_postprocess_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/blitzar
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_cli_postprocess_test)
endif()

add_executable(blitzar_bounded_contract_test
    tests/contracts/ContractBoundedTest.cpp
)
target_link_libraries(blitzar_bounded_contract_test PRIVATE blitzar)
target_compile_features(blitzar_bounded_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_bounded_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_bounded_contract_test)

add_executable(blitzar_solver_resource_contract_test
    tests/contracts/ContractSolverResourceTest.cpp
)
target_link_libraries(blitzar_solver_resource_contract_test PRIVATE blitzar)
target_compile_features(blitzar_solver_resource_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_solver_resource_contract_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(blitzar_spatial_resource_test
    tests/contracts/ContractSpatialResourceTest.cpp
)
target_link_libraries(blitzar_spatial_resource_test PRIVATE blitzar)
target_compile_features(blitzar_spatial_resource_test PRIVATE cxx_std_20)
target_include_directories(blitzar_spatial_resource_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_spatial_resource_test)
blitzar_enable_warnings(blitzar_solver_resource_contract_test)

add_executable(blitzar_force_provider_test
    tests/contracts/ContractForceProviderTest.cpp
)
target_link_libraries(blitzar_force_provider_test PRIVATE blitzar)
target_compile_features(blitzar_force_provider_test PRIVATE cxx_std_20)
target_include_directories(blitzar_force_provider_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests)
blitzar_enable_warnings(blitzar_force_provider_test)

add_executable(blitzar_abi_test
    tests/contracts/ContractAbiTest.c
)
target_link_libraries(blitzar_abi_test PRIVATE blitzar)
set_property(TARGET blitzar_abi_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_abi_test)

add_executable(blitzar_capability_test
    tests/contracts/ContractCapabilitiesTest.c
)
target_link_libraries(blitzar_capability_test PRIVATE blitzar)
set_property(TARGET blitzar_capability_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_capability_test)

add_executable(blitzar_dynamics_test
    tests/integration/KdkDynamicsTest.cpp
)
target_link_libraries(blitzar_dynamics_test PRIVATE blitzar)
target_compile_features(blitzar_dynamics_test PRIVATE cxx_std_20)
target_include_directories(blitzar_dynamics_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_dynamics_test)

add_executable(blitzar_allocation_test
    tests/fixtures/FixtureAllocationMonitor.cpp
    tests/simulation/SimAllocationsTest.cpp
)
target_link_libraries(blitzar_allocation_test PRIVATE blitzar)
target_compile_features(blitzar_allocation_test PRIVATE cxx_std_20)
target_include_directories(blitzar_allocation_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_allocation_test)

add_executable(blitzar_hierarchy_test
    tests/octree/OctreeHierarchyTest.cpp
)
target_link_libraries(blitzar_hierarchy_test PRIVATE blitzar)
target_compile_features(blitzar_hierarchy_test PRIVATE cxx_std_20)
target_include_directories(blitzar_hierarchy_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_hierarchy_test)

add_executable(blitzar_fmm_test
    tests/fixtures/FixtureAllocationMonitor.cpp
    tests/fmm/FmmTest.cpp
)
target_link_libraries(blitzar_fmm_test PRIVATE blitzar)
target_compile_features(blitzar_fmm_test PRIVATE cxx_std_20)
target_include_directories(blitzar_fmm_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_fmm_test)

add_executable(blitzar_numerical_test
    tests/integration/KdkNumericalTest.cpp
)
target_link_libraries(blitzar_numerical_test PRIVATE blitzar)
target_compile_features(blitzar_numerical_test PRIVATE cxx_std_20)
target_include_directories(blitzar_numerical_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_numerical_test)

add_executable(blitzar_conservation_test
    tests/physics/ConservationMetricsTest.cpp
)
target_link_libraries(blitzar_conservation_test PRIVATE blitzar)
target_compile_features(blitzar_conservation_test PRIVATE cxx_std_20)
target_include_directories(blitzar_conservation_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_conservation_test)

add_executable(blitzar_debug_status_probe
    tests/contracts/ContractDebugStatusTest.cpp
)
target_link_libraries(blitzar_debug_status_probe PRIVATE blitzar)
target_compile_features(blitzar_debug_status_probe PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_debug_status_probe)

add_executable(blitzar_accelerator_test
    tests/gpu/GpuHipTest.cpp
)
target_link_libraries(blitzar_accelerator_test PRIVATE blitzar)
target_compile_features(blitzar_accelerator_test PRIVATE cxx_std_20)
target_include_directories(blitzar_accelerator_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_accelerator_test)

add_executable(blitzar_scaling_test
    tests/fixtures/FixtureAllocationMonitor.cpp
    tests/scaling/ScaleTest.cpp
    tests/scaling/ScaleRunTest.cpp
    tests/scaling/ScaleWorkloadTest.cpp
)
target_link_libraries(blitzar_scaling_test PRIVATE blitzar)
target_compile_features(blitzar_scaling_test PRIVATE cxx_std_20)
target_include_directories(blitzar_scaling_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_scaling_test)

if(WIN32)
    target_link_libraries(blitzar_scaling_test PRIVATE psapi)
endif()

add_executable(blitzar_layout_test
    tests/layout/LayoutRunner.cpp
    tests/layout/LayoutState.cpp
    tests/layout/LayoutOrder.cpp
    tests/layout/LayoutStorage.cpp
    tests/layout/LayoutBenchmark.cpp
)
target_link_libraries(blitzar_layout_test PRIVATE blitzar)
target_compile_features(blitzar_layout_test PRIVATE cxx_std_20)
target_include_directories(blitzar_layout_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_layout_test)

add_executable(blitzar_reduction_test
    tests/reduction/ReductionRunner.cpp
    tests/reduction/ReductionBenchmark.cpp
)
target_link_libraries(blitzar_reduction_test PRIVATE blitzar)
target_compile_features(blitzar_reduction_test PRIVATE cxx_std_20)
target_include_directories(blitzar_reduction_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_reduction_test)

if(BLITZAR_MPI_ENABLED)
    add_executable(blitzar_mpi_test
        tests/fixtures/FixtureAllocationMonitor.cpp
        tests/mpi/MpiAllocationTest.cpp
        tests/mpi/MpiTest.cpp
        tests/mpi/MpiDomainTest.cpp
        tests/mpi/MpiExchangeTest.cpp
        tests/mpi/MpiFixture.cpp
        tests/mpi/MpiInvalidTest.cpp
        tests/mpi/MpiOverlapTest.cpp
        tests/mpi/MpiRollbackTest.cpp
        tests/mpi/MpiValidationTest.cpp
        tests/mpi/MpiWireTest.cpp
    )
    target_link_libraries(blitzar_mpi_test PRIVATE blitzar)
    target_compile_features(blitzar_mpi_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_mpi_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_mpi_test)
    blitzar_enable_mpi(blitzar_mpi_test)
endif()

set(BLITZAR_TEST_TARGETS
    blitzar_public_header_compile
    blitzar_lifecycle_test
    blitzar_config_test
    blitzar_config_run_test
    blitzar_c_api_test
    blitzar_contract_test
    blitzar_snapshot_contract_test
    blitzar_snapshot_io_test
    blitzar_metadata_io_test
    blitzar_cli_output_test
    blitzar_cli_summary_test
    blitzar_cli_restart_test
    blitzar_cli_process_restart_test
    blitzar_cli_postprocess_test
    blitzar_bounded_contract_test
    blitzar_solver_resource_contract_test
    blitzar_spatial_resource_test
    blitzar_abi_test
    blitzar_capability_test
    blitzar_dynamics_test
    blitzar_allocation_test
    blitzar_hierarchy_test
    blitzar_fmm_test
    blitzar_numerical_test
    blitzar_conservation_test
    blitzar_debug_status_probe
    blitzar_accelerator_test
    blitzar_scaling_test
    blitzar_layout_test
    blitzar_reduction_test
    blitzar_mpi_test
)
foreach(target IN LISTS BLITZAR_TEST_TARGETS)
    if(TARGET ${target})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    endif()
endforeach()
