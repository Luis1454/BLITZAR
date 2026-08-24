cmake_minimum_required(VERSION 3.25)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/plan/manifest.json" BLITZAR_PLAN_MANIFEST)
string(JSON BLITZAR_PRODUCT_VERSION
    GET "${BLITZAR_PLAN_MANIFEST}" product_version)
string(JSON BLITZAR_PLAN_VERSION
    GET "${BLITZAR_PLAN_MANIFEST}" plan_version)
if(NOT BLITZAR_PRODUCT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "plan/manifest.json product_version must be semantic version")
endif()
if(NOT BLITZAR_PLAN_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "plan/manifest.json plan_version must be semantic version")
endif()

project(BLITZAR VERSION ${BLITZAR_PRODUCT_VERSION} LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/HIP.cmake)
include(cmake/MPI.cmake)

option(BLITZAR_BUILD_TESTS "Build BLITZAR tests" ON)
option(BLITZAR_BUILD_CLI "Build the BLITZAR command-line application" ON)
option(BLITZAR_BUILD_EXAMPLES "Build SDK consumption examples" ON)
option(BLITZAR_BUILD_SHARED "Build BLITZAR as a shared library" OFF)
option(BLITZAR_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(BLITZAR_ENABLE_OPENMP "Enable CPU OpenMP parallelism" ON)

if(BLITZAR_ENABLE_SANITIZERS AND MSVC)
    message(FATAL_ERROR "BLITZAR_ENABLE_SANITIZERS requires a Clang/GNU toolchain")
endif()

if(BLITZAR_BUILD_SHARED AND BLITZAR_BUILD_TESTS)
    message(FATAL_ERROR
        "BLITZAR_BUILD_TESTS targets internal symbols; use the static build for them")
endif()

if(BLITZAR_ENABLE_OPENMP)
    find_package(OpenMP REQUIRED COMPONENTS CXX)
endif()
find_package(Threads REQUIRED)

function(blitzar_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C>:/W4>"
            "$<$<COMPILE_LANGUAGE:C>:/WX>"
            "$<$<COMPILE_LANGUAGE:CXX>:/W4>"
            "$<$<COMPILE_LANGUAGE:CXX>:/WX>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W4>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/WX>"
        )
    else()
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C>:-Wall>"
            "$<$<COMPILE_LANGUAGE:C>:-Wextra>"
            "$<$<COMPILE_LANGUAGE:C>:-Wpedantic>"
            "$<$<COMPILE_LANGUAGE:C>:-Werror>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Werror>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wextra>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Werror>"
        )
    endif()
endfunction()

if(BLITZAR_BUILD_SHARED)
    add_library(blitzar SHARED)
    target_compile_definitions(blitzar
        PRIVATE BLITZAR_BUILDING
        PUBLIC BLITZAR_SHARED
    )
else()
    add_library(blitzar STATIC)
endif()

target_sources(blitzar PRIVATE
    src/integration/LeapfrogWorkspace.cpp
    src/particles/ParticleArena.cpp
    src/particles/ParticleBuffer.cpp
    src/particles/SourceBuffer.cpp
    src/physics/GravityLaw.cpp
    src/parallel/MpiCollectives.cpp
    src/parallel/DomainDecomposition.cpp
    src/parallel/MpiGhostTransport.cpp
    src/parallel/MpiContext.cpp
    src/parallel/MpiExchange.cpp
    src/parallel/MpiPacketTransport.cpp
    src/parallel/MpiSession.cpp
    src/parallel/MpiTypes.cpp
    src/sdk/Api.cpp
    src/sdk/CppApi.cpp
    src/sdk/Simulation.cpp
    src/sdk/SrvConfig.cpp
    src/sdk/SrvParticles.cpp
    src/sdk/SrvState.cpp
    src/sdk/SrvStep.cpp
    src/sdk/SrvTransaction.cpp
    src/solvers/barnes_hut/BarnesHutSolver.cpp
    src/solvers/barnes_hut/ThreadWorkspace.cpp
    src/solvers/direct/DirectSolver.cpp
    src/trees/Octree.cpp
)

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

set(BLITZAR_INSTALL_CMAKEDIR
    ${CMAKE_INSTALL_LIBDIR}/cmake/BLITZAR)

install(TARGETS blitzar
    EXPORT BLITZARTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES
    include/blitzar/blitzar.h
    include/blitzar/blitzar.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/blitzar
)

configure_package_config_file(
    cmake/BLITZARConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BLITZARConfig.cmake
    INSTALL_DESTINATION ${BLITZAR_INSTALL_CMAKEDIR}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BLITZARConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(EXPORT BLITZARTargets
    FILE BLITZARTargets.cmake
    NAMESPACE BLITZAR::
    DESTINATION ${BLITZAR_INSTALL_CMAKEDIR}
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BLITZARConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BLITZARConfigVersion.cmake
    DESTINATION ${BLITZAR_INSTALL_CMAKEDIR}
)

if(BLITZAR_BUILD_CLI)
    add_executable(blitzar_cli
        apps/blitzar/Main.cpp
    )
    target_link_libraries(blitzar_cli PRIVATE blitzar)
    target_compile_features(blitzar_cli PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_cli)
    install(TARGETS blitzar_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BLITZAR_BUILD_EXAMPLES)
    add_executable(blitzar_c_example
        examples/CExample.c
    )
    target_link_libraries(blitzar_c_example PRIVATE blitzar)
    set_property(TARGET blitzar_c_example PROPERTY LINKER_LANGUAGE CXX)
    blitzar_enable_warnings(blitzar_c_example)

    add_executable(blitzar_cpp_example
        examples/CppExample.cpp
    )
    target_link_libraries(blitzar_cpp_example PRIVATE blitzar)
    target_compile_features(blitzar_cpp_example PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_cpp_example)
endif()

if(BLITZAR_BUILD_TESTS)
    enable_testing()

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

    add_executable(blitzar_numerical_test
        tests/Numerical.cpp
    )
    target_link_libraries(blitzar_numerical_test PRIVATE blitzar)
    target_compile_features(blitzar_numerical_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_numerical_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_numerical_test)

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
            tests/Mpi.cpp
        )
        target_link_libraries(blitzar_mpi_test PRIVATE blitzar)
        target_compile_features(blitzar_mpi_test PRIVATE cxx_std_20)
        target_include_directories(blitzar_mpi_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
        blitzar_enable_warnings(blitzar_mpi_test)
        blitzar_enable_mpi(blitzar_mpi_test)
    endif()

    add_test(NAME TST-P0-001 COMMAND blitzar_lifecycle_test)
    add_test(NAME TST-P0-002 COMMAND blitzar_c_api_test)
    add_test(NAME TST-P0-003 COMMAND blitzar_contract_test)
    add_test(NAME TST-P0-004 COMMAND blitzar_bounded_contract_test)
    add_test(NAME TST-P0-005 COMMAND blitzar_abi_test)
    add_test(NAME TST-P1-001 COMMAND blitzar_dynamics_test)
    add_test(NAME TST-P1-003 COMMAND blitzar_allocation_test)
    add_test(NAME TST-P3-001 COMMAND blitzar_hierarchy_test)
    add_test(NAME TST-P1-002 COMMAND blitzar_numerical_test)
    add_test(NAME TST-P4-001 COMMAND blitzar_hip_test)
    if(BLITZAR_MPI_ENABLED)
        add_test(
            NAME TST-P7-001
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                ${MPIEXEC_PREFLAGS} $<TARGET_FILE:blitzar_mpi_test>
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P7-002
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
                ${MPIEXEC_PREFLAGS} $<TARGET_FILE:blitzar_mpi_test>
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P8-001
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                ${MPIEXEC_PREFLAGS} $<TARGET_FILE:blitzar_mpi_test> migration
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P7-003
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
                ${MPIEXEC_PREFLAGS} $<TARGET_FILE:blitzar_mpi_test> single
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P7-004
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                ${MPIEXEC_PREFLAGS}
                $<TARGET_FILE:blitzar_mpi_test> barnes-hut-migration
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P7-005
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                ${MPIEXEC_PREFLAGS} $<TARGET_FILE:blitzar_mpi_test> large-count
                ${MPIEXEC_POSTFLAGS}
        )
        add_test(
            NAME TST-P8-002
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                ${MPIEXEC_PREFLAGS}
                $<TARGET_FILE:blitzar_mpi_test> out-of-domain
                ${MPIEXEC_POSTFLAGS}
        )
        set_tests_properties(
            TST-P7-001 TST-P7-002 TST-P7-003 TST-P7-004 TST-P7-005
            TST-P8-001 TST-P8-002
            PROPERTIES TIMEOUT 120
        )
    endif()
endif()

if(BLITZAR_BUILD_TESTS)
    if(BLITZAR_BUILD_EXAMPLES)
        add_test(NAME TST-P2-001 COMMAND blitzar_c_example)
        add_test(NAME TST-P2-002 COMMAND blitzar_cpp_example)
    endif()
    if(BLITZAR_BUILD_CLI)
        add_test(NAME TST-P2-003 COMMAND blitzar_cli)
    endif()
    add_test(
        NAME TST-P2-004
        COMMAND cmake
            -DBLITZAR_BUILD_DIR=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/PackageConsumer.cmake
    )
endif()
