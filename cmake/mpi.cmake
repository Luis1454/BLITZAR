set(BLITZAR_MPI_MODE "AUTO" CACHE STRING
    "MPI backend mode: AUTO, ON, or OFF")
set_property(CACHE BLITZAR_MPI_MODE PROPERTY STRINGS AUTO ON OFF)

string(TOUPPER "${BLITZAR_MPI_MODE}" _blitzar_mpi_mode)
if(NOT _blitzar_mpi_mode STREQUAL "AUTO" AND
   NOT _blitzar_mpi_mode STREQUAL "ON" AND
   NOT _blitzar_mpi_mode STREQUAL "OFF")
    message(FATAL_ERROR "BLITZAR_MPI_MODE must be AUTO, ON, or OFF")
endif()

set(BLITZAR_MPI_ENABLED OFF CACHE INTERNAL "Whether MPI is enabled")

if(NOT _blitzar_mpi_mode STREQUAL "OFF")
    find_package(MPI QUIET COMPONENTS CXX)
    if(MPI_CXX_FOUND)
        set(BLITZAR_MPI_ENABLED ON CACHE INTERNAL "Whether MPI is enabled" FORCE)
        message(STATUS "BLITZAR MPI backend enabled with ${MPI_CXX_COMPILER}")
    elseif(_blitzar_mpi_mode STREQUAL "ON")
        message(FATAL_ERROR "BLITZAR_MPI_MODE=ON requires MPI CXX")
    else()
        message(STATUS "BLITZAR MPI backend unavailable; using single-rank execution")
    endif()
else()
    message(STATUS "BLITZAR MPI backend disabled")
endif()

function(blitzar_enable_mpi target)
    if(BLITZAR_MPI_ENABLED)
        target_compile_definitions(${target} PRIVATE
            BLITZAR_HAS_MPI=1
            BLITZAR_COMPILED_MPI=1
        )
        target_link_libraries(${target} PUBLIC MPI::MPI_CXX)
    endif()
endfunction()
