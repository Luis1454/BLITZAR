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
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/qt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/physics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/physics/cuda/fragments"
)

set(GRAVITY_BACKEND_SOURCES
    engine/src/config/EnvUtils.cpp
    engine/src/config/SimulationArgs.cpp
    engine/src/config/SimulationArgsParse.cpp
    engine/src/config/SimulationArgsCoreOptions.cpp
    engine/src/config/SimulationArgsFrontendOptions.cpp
    engine/src/config/SimulationArgsInitOptions.cpp
    engine/src/config/SimulationArgsInitStateOptions.cpp
    engine/src/config/SimulationArgsFluidOptions.cpp
    engine/src/config/SimulationOptionRegistry.cpp
    engine/src/config/SimulationOptionRegistryApply.cpp
    engine/src/config/SimulationOptionRegistryEntries.cpp
    engine/src/config/SimulationConfig.cpp
    engine/src/config/SimulationModes.cpp
    engine/src/config/TextParse.cpp
    engine/src/backend/SimulationBackend.cpp
    engine/src/backend/SimulationInitConfig.cpp
    engine/src/physics/cuda/ParticleSystem.cu
)

set(GRAVITY_RUNTIME_PROTOCOL_SOURCES
    runtime/src/protocol/BackendJsonCodec.cpp
    runtime/src/protocol/BackendJsonCodecParse.cpp
    runtime/src/protocol/BackendJsonCodecParseStatus.cpp
    runtime/src/protocol/BackendJsonCodecParseSnapshot.cpp
    runtime/src/protocol/BackendJsonCodecReadNumber.cpp
    runtime/src/protocol/BackendClient.cpp
    runtime/src/protocol/BackendProtocol.cpp
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
