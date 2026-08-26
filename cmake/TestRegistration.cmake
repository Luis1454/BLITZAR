enable_testing()

add_test(NAME TST-P0-001 COMMAND blitzar_lifecycle_test)
add_test(NAME TST-P0-002 COMMAND blitzar_c_api_test)
add_test(NAME TST-P0-003 COMMAND blitzar_contract_test)
add_test(NAME TST-P0-004 COMMAND blitzar_bounded_contract_test)
add_test(NAME TST-P0-005 COMMAND blitzar_abi_test)
add_test(NAME TST-P0-006 COMMAND blitzar_capability_test)
add_test(NAME TST-P1-001 COMMAND blitzar_dynamics_test)
add_test(NAME TST-P1-003 COMMAND blitzar_allocation_test)
add_test(NAME TST-P3-001 COMMAND blitzar_hierarchy_test)
add_test(NAME TST-P3-002 COMMAND blitzar_fmm_test)
add_test(NAME TST-P1-002 COMMAND blitzar_numerical_test)
add_test(NAME TST-P4-001 COMMAND blitzar_accelerator_test)
add_test(
    NAME TST-P0-007
    COMMAND blitzar_scaling_test --particles 16 --steps 1 --seed 424242 --solver direct
)

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
    add_test(
        NAME TST-P8-003
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
            ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:blitzar_mpi_test> overlap
            ${MPIEXEC_POSTFLAGS}
    )
    set_tests_properties(
        TST-P7-001 TST-P7-002 TST-P7-003 TST-P7-004 TST-P7-005
        TST-P8-001 TST-P8-002 TST-P8-003
        PROPERTIES TIMEOUT 120
    )
endif()

if(BLITZAR_BUILD_EXAMPLES)
    add_test(NAME TST-P2-001 COMMAND blitzar_c_example)
    add_test(NAME TST-P2-002 COMMAND blitzar_cpp_example)
    add_test(NAME TST-P2-005 COMMAND blitzar_c_v2_example)
endif()

if(BLITZAR_BUILD_CLI)
    add_test(NAME TST-P2-003 COMMAND blitzar_cli)
endif()

add_test(
    NAME TST-P2-004
    COMMAND cmake
        -DBLITZAR_BUILD_DIR=${CMAKE_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/PackageConsumer.cmake
)
