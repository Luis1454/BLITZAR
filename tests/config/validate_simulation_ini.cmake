cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED GRAVITY_CONFIG_PATH)
    set(GRAVITY_CONFIG_PATH "simulation.ini")
endif()

if(NOT EXISTS "${GRAVITY_CONFIG_PATH}")
    message(FATAL_ERROR "missing config file: ${GRAVITY_CONFIG_PATH}")
endif()

file(READ "${GRAVITY_CONFIG_PATH}" GRAVITY_CONFIG_CONTENT)
string(REPLACE "\r\n" "\n" GRAVITY_CONFIG_CONTENT "${GRAVITY_CONFIG_CONTENT}")
string(REPLACE "\r" "\n" GRAVITY_CONFIG_CONTENT "${GRAVITY_CONFIG_CONTENT}")

set(GRAVITY_CONFIG_ERRORS "")

function(gravity_get_value key out_var found_var)
    string(REGEX MATCH "(^|\n)[ \t]*${key}[ \t]*=([^\n]*)" _line "${GRAVITY_CONFIG_CONTENT}")
    if(_line)
        string(REGEX REPLACE "(^|\n)[ \t]*${key}[ \t]*=([^\n]*)" "\\2" _value "${_line}")
        string(STRIP "${_value}" _value)
        set(${out_var} "${_value}" PARENT_SCOPE)
        set(${found_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
        set(${found_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(gravity_append_error text)
    list(APPEND GRAVITY_CONFIG_ERRORS "${text}")
    set(GRAVITY_CONFIG_ERRORS "${GRAVITY_CONFIG_ERRORS}" PARENT_SCOPE)
endfunction()

function(gravity_require_key key)
    gravity_get_value("${key}" _value _found)
    if(NOT _found)
        gravity_append_error("${key}: missing key")
    endif()
endfunction()

function(gravity_check_int_min key min_value)
    gravity_get_value("${key}" _value _found)
    if(NOT _found)
        return()
    endif()
    if(NOT _value MATCHES "^-?[0-9]+$")
        gravity_append_error("${key}: expected integer, got '${_value}'")
        return()
    endif()
    if(_value LESS ${min_value})
        gravity_append_error("${key}: expected >= ${min_value}, got ${_value}")
    endif()
endfunction()

function(gravity_check_float_min key min_value)
    gravity_get_value("${key}" _value _found)
    if(NOT _found)
        return()
    endif()
    if(NOT _value MATCHES "^-?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][-+]?[0-9]+)?$")
        gravity_append_error("${key}: expected float, got '${_value}'")
        return()
    endif()
    if(_value LESS ${min_value})
        gravity_append_error("${key}: expected >= ${min_value}, got ${_value}")
    endif()
endfunction()

function(gravity_check_enum key)
    set(options "${ARGN}")
    gravity_get_value("${key}" _value _found)
    if(NOT _found)
        return()
    endif()
    list(FIND options "${_value}" _index)
    if(_index EQUAL -1)
        gravity_append_error("${key}: expected one of [${options}], got '${_value}'")
    endif()
endfunction()

set(_required_keys
    particle_count
    dt
    solver
    integrator
    octree_theta
    octree_softening
    frontend_particle_cap
    ui_fps_limit
    export_format
    input_format
    init_config_style
    preset_structure
    init_mode
    sph_enabled
)

foreach(_key IN LISTS _required_keys)
    gravity_require_key("${_key}")
endforeach()

gravity_check_int_min(particle_count 1)
gravity_check_int_min(frontend_particle_cap 1)
gravity_check_int_min(ui_fps_limit 1)
gravity_check_int_min(energy_measure_every_steps 1)
gravity_check_int_min(energy_sample_limit 1)

gravity_check_float_min(dt 0.0)
gravity_check_float_min(octree_theta 0.0)
gravity_check_float_min(octree_softening 0.0)
gravity_check_float_min(default_zoom 0.0)
gravity_check_float_min(default_luminosity 0.0)

gravity_check_enum(solver pairwise_cuda octree_gpu octree_cpu)
gravity_check_enum(integrator euler rk4)
gravity_check_enum(export_format vtk vtk_binary xyz bin)
gravity_check_enum(input_format auto vtk vtk_binary xyz bin)
gravity_check_enum(init_config_style preset detailed)
gravity_check_enum(preset_structure disk_orbit random_cloud file)
gravity_check_enum(init_mode disk_orbit random_cloud file)
gravity_check_enum(sph_enabled true false)

if(GRAVITY_CONFIG_ERRORS)
    list(JOIN GRAVITY_CONFIG_ERRORS "\n  - " _joined)
    message(FATAL_ERROR "simulation.ini validation failed:\n  - ${_joined}")
endif()

message(STATUS "simulation.ini validation OK")
