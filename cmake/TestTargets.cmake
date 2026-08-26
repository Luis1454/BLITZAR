add_library(blitzar_public_header_compile OBJECT
    tests/PublicC.c
    tests/PublicCpp.cpp
)
target_include_directories(blitzar_public_header_compile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(blitzar_public_header_compile PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_public_header_compile)

add_executable(blitzar_lifecycle_test
    tests/Lifecycle.cpp
)
target_link_libraries(blitzar_lifecycle_test PRIVATE blitzar)
target_link_libraries(blitzar_lifecycle_test PRIVATE Threads::Threads)
target_compile_features(blitzar_lifecycle_test PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_lifecycle_test)

add_executable(blitzar_c_api_test
    tests/ApiC.c
)
target_link_libraries(blitzar_c_api_test PRIVATE blitzar)
set_property(TARGET blitzar_c_api_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_c_api_test)

add_executable(blitzar_contract_test
    tests/Contracts.cpp
)
target_link_libraries(blitzar_contract_test PRIVATE blitzar)
target_compile_features(blitzar_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_contract_test)

add_executable(blitzar_bounded_contract_test
    tests/Bounded.cpp
)
target_link_libraries(blitzar_bounded_contract_test PRIVATE blitzar)
target_compile_features(blitzar_bounded_contract_test PRIVATE cxx_std_20)
target_include_directories(blitzar_bounded_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_bounded_contract_test)

add_executable(blitzar_abi_test
    tests/Abi.c
)
target_link_libraries(blitzar_abi_test PRIVATE blitzar)
set_property(TARGET blitzar_abi_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_abi_test)

add_executable(blitzar_capability_test
    tests/Capabilities.c
)
target_link_libraries(blitzar_capability_test PRIVATE blitzar)
set_property(TARGET blitzar_capability_test PROPERTY LINKER_LANGUAGE CXX)
blitzar_enable_warnings(blitzar_capability_test)

add_executable(blitzar_dynamics_test
    tests/Dynamics.cpp
)
target_link_libraries(blitzar_dynamics_test PRIVATE blitzar)
target_compile_features(blitzar_dynamics_test PRIVATE cxx_std_20)
target_include_directories(blitzar_dynamics_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_dynamics_test)

add_executable(blitzar_allocation_test
    tests/AllocationMonitor.cpp
    tests/Allocations.cpp
)
target_link_libraries(blitzar_allocation_test PRIVATE blitzar)
target_compile_features(blitzar_allocation_test PRIVATE cxx_std_20)
target_include_directories(blitzar_allocation_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_allocation_test)

add_executable(blitzar_hierarchy_test
    tests/Hierarchy.cpp
)
target_link_libraries(blitzar_hierarchy_test PRIVATE blitzar)
target_compile_features(blitzar_hierarchy_test PRIVATE cxx_std_20)
target_include_directories(blitzar_hierarchy_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_hierarchy_test)

add_executable(blitzar_fmm_test
    tests/AllocationMonitor.cpp
    tests/Fmm.cpp
)
target_link_libraries(blitzar_fmm_test PRIVATE blitzar)
target_compile_features(blitzar_fmm_test PRIVATE cxx_std_20)
target_include_directories(blitzar_fmm_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_fmm_test)

add_executable(blitzar_numerical_test
    tests/Numerical.cpp
)
target_link_libraries(blitzar_numerical_test PRIVATE blitzar)
target_compile_features(blitzar_numerical_test PRIVATE cxx_std_20)
target_include_directories(blitzar_numerical_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_numerical_test)

add_executable(blitzar_debug_status_probe
    tests/DebugStatus.cpp
)
target_link_libraries(blitzar_debug_status_probe PRIVATE blitzar)
target_compile_features(blitzar_debug_status_probe PRIVATE cxx_std_20)
blitzar_enable_warnings(blitzar_debug_status_probe)

add_executable(blitzar_hip_test
    tests/Hip.cpp
)
target_link_libraries(blitzar_hip_test PRIVATE blitzar)
target_compile_features(blitzar_hip_test PRIVATE cxx_std_20)
target_include_directories(blitzar_hip_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
blitzar_enable_warnings(blitzar_hip_test)

if(BLITZAR_MPI_ENABLED)
    add_executable(blitzar_mpi_test
        tests/AllocationMonitor.cpp
        tests/MpiAllocationTest.cpp
        tests/Mpi.cpp
        tests/MpiDomain.cpp
        tests/MpiExchangeTest.cpp
        tests/MpiFixture.cpp
        tests/MpiInvalidTest.cpp
        tests/MpiOverlapTest.cpp
        tests/MpiRollbackTest.cpp
        tests/MpiValidationTest.cpp
        tests/MpiWireTest.cpp
    )
    target_link_libraries(blitzar_mpi_test PRIVATE blitzar)
    target_compile_features(blitzar_mpi_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_mpi_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_mpi_test)
    blitzar_enable_mpi(blitzar_mpi_test)
endif()
