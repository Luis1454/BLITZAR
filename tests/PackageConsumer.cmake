if(NOT DEFINED BLITZAR_BUILD_DIR OR NOT IS_DIRECTORY "${BLITZAR_BUILD_DIR}")
    message(FATAL_ERROR "BLITZAR_BUILD_DIR must name a configured build directory")
endif()

set(test_root "${BLITZAR_BUILD_DIR}/package-consumer")
set(install_prefix "${test_root}/install")
set(source_dir "${test_root}/source")
set(consumer_build_dir "${test_root}/build")
file(REMOVE_RECURSE "${test_root}")
file(MAKE_DIRECTORY "${source_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BLITZAR_BUILD_DIR}"
        --prefix "${install_prefix}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR
        "BLITZAR install failed:\n${install_output}\n${install_error}")
endif()

file(WRITE "${source_dir}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.25)
project(BLITZARPackageConsumer LANGUAGES CXX)

find_package(BLITZAR CONFIG REQUIRED)

add_executable(package_consumer main.cpp)
target_link_libraries(package_consumer PRIVATE BLITZAR::blitzar)
]=])

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
        -DCMAKE_BUILD_TYPE=Release
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
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR
        "BLITZAR package consumer build failed:\n"
        "${build_output}\n${build_error}")
endif()
