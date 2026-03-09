if(NOT DEFINED GRAVITY_SOURCE_ROOT)
    set(GRAVITY_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
get_filename_component(GRAVITY_SOURCE_ROOT "${GRAVITY_SOURCE_ROOT}" ABSOLUTE)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

gravity_populate_windows_toolchain_hints()
find_package(CUDAToolkit REQUIRED)

set(GRAVITY_PROJECT_INCLUDE_DIRS
    "${GRAVITY_SOURCE_ROOT}"
    "${GRAVITY_SOURCE_ROOT}/engine/include"
    "${GRAVITY_SOURCE_ROOT}/runtime/include"
    "${GRAVITY_SOURCE_ROOT}/modules/qt/include"
    "${GRAVITY_SOURCE_ROOT}/engine/include/physics"
    "${GRAVITY_SOURCE_ROOT}/engine/src/physics/cuda/fragments"
)

if(WIN32)
    set(GRAVITY_ENV_UTILS_SOURCES
        "${GRAVITY_SOURCE_ROOT}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_SOURCE_ROOT}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(GRAVITY_ENV_UTILS_SOURCES
        "${GRAVITY_SOURCE_ROOT}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_SOURCE_ROOT}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()

set(GRAVITY_BACKEND_SOURCES
    ${GRAVITY_ENV_UTILS_SOURCES}
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgs.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsParse.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsCoreOptions.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsFrontendOptions.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsInitOptions.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsInitStateOptions.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationArgsFluidOptions.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationOptionRegistry.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationConfig.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/SimulationModes.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/config/TextParse.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/backend/SimulationBackend.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/backend/SimulationInitConfig.cpp"
    "${GRAVITY_SOURCE_ROOT}/engine/src/physics/cuda/ParticleSystem.cu"
)

set(GRAVITY_RUNTIME_PROTOCOL_SOURCES
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendJsonCodec.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendJsonCodecParse.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendJsonCodecReadNumber.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendJsonCodecParseStatus.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendJsonCodecParseSnapshot.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendClient.cpp"
    "${GRAVITY_SOURCE_ROOT}/runtime/src/protocol/BackendProtocol.cpp"
)

function(gravity_collect_existing_paths out_var)
    set(_result "")
    foreach(_path IN LISTS ARGN)
        if(NOT "${_path}" STREQUAL "" AND EXISTS "${_path}")
            list(APPEND _result "${_path}")
        endif()
    endforeach()
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

if(WIN32)
    gravity_collect_existing_paths(
        GRAVITY_WINDOWS_SYSTEM_INCLUDES
        "${GRAVITY_MSVC_INCLUDE_DIR}"
        "${GRAVITY_WINSDK_INCLUDE_UCRT}"
        "${GRAVITY_WINSDK_INCLUDE_UM}"
        "${GRAVITY_WINSDK_INCLUDE_SHARED}"
        "${GRAVITY_WINSDK_INCLUDE_WINRT}"
        "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}"
    )
    gravity_collect_existing_paths(
        GRAVITY_WINDOWS_LINK_DIRS
        "${GRAVITY_MSVC_LIB_DIR}"
        "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
        "${GRAVITY_WINSDK_UM_LIB_DIR}"
    )
endif()
