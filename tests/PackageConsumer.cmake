if(NOT DEFINED BLITZAR_BUILD_DIR OR NOT IS_DIRECTORY "${BLITZAR_BUILD_DIR}")
    message(FATAL_ERROR "BLITZAR_BUILD_DIR must name a configured build directory")
endif()

set(manifest_file "${CMAKE_CURRENT_LIST_DIR}/../plan/manifest.json")
file(READ "${manifest_file}" manifest_json)
string(JSON expected_product_version
    GET "${manifest_json}" product_version)
string(JSON expected_plan_version
    GET "${manifest_json}" plan_version)

set(test_root "${BLITZAR_BUILD_DIR}/package-consumer")
set(install_prefix "${test_root}/install")
set(source_dir "${test_root}/source")
set(consumer_build_dir "${test_root}/build")
file(REMOVE_RECURSE "${test_root}")
file(MAKE_DIRECTORY "${source_dir}")

set(package_config "Release")
set(cache_file "${BLITZAR_BUILD_DIR}/CMakeCache.txt")
if(EXISTS "${cache_file}")
    file(STRINGS "${cache_file}" build_type_entry
        REGEX "^CMAKE_BUILD_TYPE:STRING=" LIMIT_COUNT 1)
    if(build_type_entry)
        string(REGEX REPLACE "^CMAKE_BUILD_TYPE:STRING=" ""
            package_config "${build_type_entry}")
    endif()
endif()
if(package_config STREQUAL "")
    set(package_config "Release")
endif()

set(consumer_generator "")
set(generator_entry "")
file(STRINGS "${cache_file}" generator_entry
    REGEX "^CMAKE_GENERATOR:INTERNAL=" LIMIT_COUNT 1)
if(generator_entry)
    string(REGEX REPLACE "^CMAKE_GENERATOR:INTERNAL=" ""
        consumer_generator "${generator_entry}")
endif()

set(consumer_compiler "")
set(compiler_entry "")
file(STRINGS "${cache_file}" compiler_entry
    REGEX "^CMAKE_CXX_COMPILER:(FILEPATH|STRING)=" LIMIT_COUNT 1)
if(compiler_entry)
    string(REGEX REPLACE "^CMAKE_CXX_COMPILER:(FILEPATH|STRING)=" ""
        consumer_compiler "${compiler_entry}")
endif()

set(consumer_multi_config FALSE)
set(configuration_types_entry "")
file(STRINGS "${cache_file}" configuration_types_entry
    REGEX "^CMAKE_CONFIGURATION_TYPES:(INTERNAL|STRING)=" LIMIT_COUNT 1)
if(configuration_types_entry)
    set(consumer_multi_config TRUE)
endif()

set(example_c "${CMAKE_CURRENT_LIST_DIR}/../examples/CExample.c")
set(example_cpp "${CMAKE_CURRENT_LIST_DIR}/../examples/CppExample.cpp")
if(NOT EXISTS "${example_c}" OR NOT EXISTS "${example_cpp}")
    message(FATAL_ERROR "public SDK examples are missing")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BLITZAR_BUILD_DIR}"
        --prefix "${install_prefix}"
        --config "${package_config}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR
        "BLITZAR install failed:\n${install_output}\n${install_error}")
endif()

set(consumer_cmake [=[
cmake_minimum_required(VERSION 3.25)
project(BLITZARPackageConsumer LANGUAGES C CXX)

find_package(BLITZAR CONFIG REQUIRED)

if(NOT DEFINED BLITZAR_VERSION OR
   NOT BLITZAR_VERSION STREQUAL "@EXPECTED_PRODUCT_VERSION@")
    message(FATAL_ERROR "installed BLITZAR product version is incorrect")
endif()
if(NOT DEFINED BLITZAR_PLAN_VERSION OR
   NOT BLITZAR_PLAN_VERSION STREQUAL "@EXPECTED_PLAN_VERSION@")
    message(FATAL_ERROR "installed BLITZAR plan version is incorrect")
endif()

add_executable(package_c_consumer "@EXAMPLE_C@")
set_property(TARGET package_c_consumer PROPERTY LINKER_LANGUAGE CXX)
target_link_libraries(package_c_consumer PRIVATE BLITZAR::blitzar)

add_executable(package_cpp_consumer "@EXAMPLE_CPP@")
target_link_libraries(package_cpp_consumer PRIVATE BLITZAR::blitzar)
]=])
set(EXPECTED_PRODUCT_VERSION "${expected_product_version}")
set(EXPECTED_PLAN_VERSION "${expected_plan_version}")
set(EXAMPLE_C "${example_c}")
set(EXAMPLE_CPP "${example_cpp}")
string(CONFIGURE "${consumer_cmake}" consumer_cmake @ONLY)
file(WRITE "${source_dir}/CMakeLists.txt" "${consumer_cmake}")

set(consumer_configure_command
    "${CMAKE_COMMAND}" -S "${source_dir}" -B "${consumer_build_dir}"
    "-DCMAKE_PREFIX_PATH=${install_prefix}"
    "-DCMAKE_BUILD_TYPE=${package_config}")
if(NOT consumer_generator STREQUAL "")
    list(APPEND consumer_configure_command -G "${consumer_generator}")
endif()
if(NOT consumer_compiler STREQUAL "")
    list(APPEND consumer_configure_command
        "-DCMAKE_CXX_COMPILER=${consumer_compiler}")
endif()

execute_process(
    COMMAND ${consumer_configure_command}
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "BLITZAR package consumer configure failed:\n"
        "${configure_output}\n${configure_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build_dir}"
        --config "${package_config}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR
        "BLITZAR package consumer build failed:\n"
        "${build_output}\n${build_error}")
endif()

set(path_separator ":")
if(WIN32)
    set(path_separator ";")
endif()
set(runtime_path
    "${install_prefix}/bin${path_separator}${install_prefix}/lib${path_separator}$ENV{PATH}")
set(runtime_path_argument "${runtime_path}")
if(WIN32)
    string(REPLACE ";" "\\;" runtime_path_argument "${runtime_path}")
endif()
set(consumer_executable_dir "${consumer_build_dir}")
if(consumer_multi_config)
    set(consumer_executable_dir "${consumer_build_dir}/${package_config}")
endif()
if(WIN32)
    set(executable_suffix ".exe")
else()
    set(executable_suffix "")
endif()

set(run_environment "${CMAKE_COMMAND}" -E env "PATH=${runtime_path_argument}")
if(NOT WIN32)
    list(APPEND run_environment
        "LD_LIBRARY_PATH=${install_prefix}/lib${path_separator}$ENV{LD_LIBRARY_PATH}")
endif()

foreach(consumer_executable IN ITEMS
        "${consumer_executable_dir}/package_c_consumer${executable_suffix}"
        "${consumer_executable_dir}/package_cpp_consumer${executable_suffix}")
    execute_process(
        COMMAND ${run_environment} "${consumer_executable}"
        RESULT_VARIABLE run_result
        OUTPUT_VARIABLE run_output
        ERROR_VARIABLE run_error)
    if(NOT run_result EQUAL 0)
        message(FATAL_ERROR
            "BLITZAR package consumer execution failed for ${consumer_executable}:\n"
            "${run_output}\n${run_error}")
    endif()
endforeach()
