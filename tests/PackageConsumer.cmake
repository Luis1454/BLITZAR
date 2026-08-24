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
project(BLITZARPackageConsumer LANGUAGES CXX)

find_package(BLITZAR CONFIG REQUIRED)

if(NOT DEFINED BLITZAR_VERSION OR
   NOT BLITZAR_VERSION STREQUAL "@EXPECTED_PRODUCT_VERSION@")
    message(FATAL_ERROR "installed BLITZAR product version is incorrect")
endif()
if(NOT DEFINED BLITZAR_PLAN_VERSION OR
   NOT BLITZAR_PLAN_VERSION STREQUAL "@EXPECTED_PLAN_VERSION@")
    message(FATAL_ERROR "installed BLITZAR plan version is incorrect")
endif()

add_executable(package_consumer main.cpp)
target_link_libraries(package_consumer PRIVATE BLITZAR::blitzar)
]=])
set(EXPECTED_PRODUCT_VERSION "${expected_product_version}")
set(EXPECTED_PLAN_VERSION "${expected_plan_version}")
string(CONFIGURE "${consumer_cmake}" consumer_cmake @ONLY)
file(WRITE "${source_dir}/CMakeLists.txt" "${consumer_cmake}")

file(WRITE "${source_dir}/main.cpp" [=[
#include <blitzar/blitzar.hpp>

int main()
{
    blitzar::Context context;
    return context.valid() ? 0 : 1;
}
]=])

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${source_dir}" -B "${consumer_build_dir}"
        "-DCMAKE_PREFIX_PATH=${install_prefix}"
        "-DCMAKE_BUILD_TYPE=${package_config}"
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
