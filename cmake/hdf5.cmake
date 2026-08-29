set(BLITZAR_HDF5_MODE "AUTO" CACHE STRING
    "HDF5 snapshot adapter mode: AUTO, ON, or OFF")
set_property(CACHE BLITZAR_HDF5_MODE PROPERTY STRINGS AUTO ON OFF)

string(TOUPPER "${BLITZAR_HDF5_MODE}" _blitzar_hdf5_mode)
if(NOT _blitzar_hdf5_mode STREQUAL "AUTO" AND
   NOT _blitzar_hdf5_mode STREQUAL "ON" AND
   NOT _blitzar_hdf5_mode STREQUAL "OFF")
    message(FATAL_ERROR "BLITZAR_HDF5_MODE must be AUTO, ON, or OFF")
endif()

set(BLITZAR_HDF5_ENABLED OFF CACHE INTERNAL
    "Whether the optional HDF5 snapshot adapter is enabled" FORCE)
set(BLITZAR_HDF5_INCLUDE_DIRS "" CACHE INTERNAL
    "HDF5 include directories used by BLITZAR" FORCE)
set(BLITZAR_HDF5_LIBRARIES "" CACHE INTERNAL
    "HDF5 libraries used by BLITZAR" FORCE)

if(NOT _blitzar_hdf5_mode STREQUAL "OFF")
    find_package(HDF5 QUIET COMPONENTS C)

    if(HDF5_C_FOUND)
        set(BLITZAR_HDF5_ENABLED ON CACHE INTERNAL
            "Whether the optional HDF5 snapshot adapter is enabled" FORCE)
        set(BLITZAR_HDF5_INCLUDE_DIRS "${HDF5_INCLUDE_DIRS}" CACHE INTERNAL
            "HDF5 include directories used by BLITZAR" FORCE)
        set(BLITZAR_HDF5_LIBRARIES "${HDF5_C_LIBRARIES}" CACHE INTERNAL
            "HDF5 libraries used by BLITZAR" FORCE)
        message(STATUS "BLITZAR HDF5 snapshot adapter enabled")
    elseif(_blitzar_hdf5_mode STREQUAL "ON")
        message(FATAL_ERROR "BLITZAR_HDF5_MODE=ON requires HDF5 C support")
    else()
        message(STATUS "HDF5 is unavailable; using the binary snapshot codec")
    endif()
else()
    message(STATUS "HDF5 snapshot adapter disabled")
endif()

function(blitzar_enable_hdf5 target)
    if(NOT BLITZAR_HDF5_ENABLED)
        return()
    endif()

    target_compile_definitions(${target} PRIVATE
        BLITZAR_HAS_HDF5=1
        BLITZAR_COMPILED_HDF5=1
    )

    if(TARGET HDF5::HDF5)
        target_link_libraries(${target} PUBLIC HDF5::HDF5)
    else()
        target_include_directories(${target} SYSTEM PRIVATE ${BLITZAR_HDF5_INCLUDE_DIRS})
        target_link_libraries(${target} PUBLIC ${BLITZAR_HDF5_LIBRARIES})
    endif()
endfunction()
