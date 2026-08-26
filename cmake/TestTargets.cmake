add_library(blitzar_public_header_compile OBJECT
    tests/contracts/PublicC.c
    tests/contracts/PublicCpp.cpp
)
target_include_directories(blitzar_public_header_compile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(blitzar_public_header_compile PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_public_header_compile)

add_executable(blitzar_lifecycle_test
    tests/simulation/Lifecycle.cpp
)
target_link_libraries(blitzar_lifecycle_test PRIVATE blitzar)
target_link_libraries(blitzar_lifecycle_test PRIVATE Threads::Threads)
target_compile_features(blitzar_lifecycle_test PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_lifecycle_test)

add_executable(blitzar_c_api_test
    tests/contracts/ApiC.c
)
target_link_libraries(blitzar_c_api_test PRIVATE blitzar)
set_property(TARGET blitzar_c_api_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_c_api_test)

add_executable(blitzar_contract_test
    tests/contracts/Contracts.cpp
)
target_link_libraries(blitzar_contract_test PRIVATE blitzar)
target_compile_features(blitzar_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_contract_test)

add_executable(blitzar_bounded_contract_test
    tests/contracts/Bounded.cpp
)
target_link_libraries(blitzar_bounded_contract_test PRIVATE blitzar)
target_compile_features(blitzar_bounded_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_bounded_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_bounded_contract_test)

add_executable(blitzar_abi_test
    tests/contracts/Abi.c
)
target_link_libraries(blitzar_abi_test PRIVATE blitzar)
set_property(TARGET blitzar_abi_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_abi_test)

add_executable(blitzar_capability_test
    tests/contracts/Capabilities.c
)
target_link_libraries(blitzar_capability_test PRIVATE blitzar)
set_property(TARGET blitzar_capability_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_capability_test)

add_executable(blitzar_dynamics_test
    tests/integration/Dynamics.cpp
)
target_link_libraries(blitzar_dynamics_test PRIVATE blitzar)
target_compile_features(blitzar_dynamics_test PRIVATE cxx_std_20)
target_include_directories(blitzar_dynamics_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_dynamics_test)

add_executable(blitzar_allocation_test
    tests/fixtures/AllocationMonitor.cpp
    tests/simulation/Allocations.cpp
)
target_link_libraries(blitzar_allocation_test PRIVATE blitzar)
target_compile_features(blitzar_allocation_test PRIVATE cxx_std_20)
target_include_directories(blitzar_allocation_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_allocation_test)

add_executable(blitzar_hierarchy_test
    tests/trees/octree/Hierarchy.cpp
)
target_link_libraries(blitzar_hierarchy_test PRIVATE blitzar)
target_compile_features(blitzar_hierarchy_test PRIVATE cxx_std_20)
target_include_directories(blitzar_hierarchy_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_hierarchy_test)

add_executable(blitzar_fmm_test
    tests/fixtures/AllocationMonitor.cpp
    tests/solvers/fmm/Fmm.cpp
)
target_link_libraries(blitzar_fmm_test PRIVATE blitzar)
target_compile_features(blitzar_fmm_test PRIVATE cxx_std_20)
target_include_directories(blitzar_fmm_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_fmm_test)

add_executable(blitzar_numerical_test
    tests/integration/Numerical.cpp
)
target_link_libraries(blitzar_numerical_test PRIVATE blitzar)
target_compile_features(blitzar_numerical_test PRIVATE cxx_std_20)
target_include_directories(blitzar_numerical_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_numerical_test)

add_executable(blitzar_debug_status_probe
    tests/contracts/DebugStatus.cpp
)
target_link_libraries(blitzar_debug_status_probe PRIVATE blitzar)
target_compile_features(blitzar_debug_status_probe PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_debug_status_probe)

add_executable(blitzar_accelerator_test
    tests/accelerators/gpu/hip/Hip.cpp
)
target_link_libraries(blitzar_accelerator_test PRIVATE blitzar)
target_compile_features(blitzar_accelerator_test PRIVATE cxx_std_20)
target_include_directories(blitzar_accelerator_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_accelerator_test)

add_executable(blitzar_scaling_test
    tests/scaling/Scaling.cpp
    tests/scaling/ScalingRun.cpp
    tests/scaling/ScalingWorkload.cpp
)
target_link_libraries(blitzar_scaling_test PRIVATE blitzar)
target_compile_features(blitzar_scaling_test PRIVATE cxx_std_20)
target_include_directories(blitzar_scaling_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_scaling_test)

if(WIN32)
    target_link_libraries(blitzar_scaling_test PRIVATE psapi)
endif()

if(BLITZAR_MPI_ENABLED)
    add_executable(blitzar_mpi_test
        tests/fixtures/AllocationMonitor.cpp
        tests/parallel/mpi/MpiAllocationTest.cpp
        tests/parallel/mpi/Mpi.cpp
        tests/parallel/mpi/MpiDomain.cpp
        tests/parallel/mpi/MpiExchangeTest.cpp
        tests/parallel/mpi/MpiFixture.cpp
        tests/parallel/mpi/MpiInvalidTest.cpp
        tests/parallel/mpi/MpiOverlapTest.cpp
        tests/parallel/mpi/MpiRollbackTest.cpp
        tests/parallel/mpi/MpiValidationTest.cpp
        tests/parallel/mpi/MpiWireTest.cpp
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
    blitzar_c_api_test
    blitzar_contract_test
    blitzar_bounded_contract_test
    blitzar_abi_test
    blitzar_capability_test
    blitzar_dynamics_test
    blitzar_allocation_test
    blitzar_hierarchy_test
    blitzar_fmm_test
    blitzar_numerical_test
    blitzar_debug_status_probe
    blitzar_accelerator_test
    blitzar_scaling_test
    blitzar_mpi_test
)
foreach(target IN LISTS BLITZAR_TEST_TARGETS)
    if(TARGET ${target})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    endif()
endforeach()
