# @file cmake/core/toolchain.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED BLITZAR_ROOT_DIR)
    get_filename_component(BLITZAR_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
endif()
# Broad architecture support (Maxwell to Lovelace) using -real to avoid PTX JIT version issues
set(CMAKE_CUDA_ARCHITECTURES 75-real 80-real 86-real 89-real)
set(CMAKE_CUDA_SEPARABLE_COMPILATION OFF)

# Ensure no device debug which causes PTX JIT version issues
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()
# Extra safety: remove -G and -g from CUDA flags if they ever appear
string(REPLACE "-G" "" CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG}")
string(REPLACE "-g" "" CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG}")

BLITZAR_populate_windows_toolchain_hints()
find_package(CUDAToolkit REQUIRED)

set(BLITZAR_PROJECT_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/qt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/physics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/graphics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/physics/cuda/fragments"
)

if(WIN32)
    set(BLITZAR_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(BLITZAR_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()

set(BLITZAR_GRAPHICS_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/src/graphics/ViewMath.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/graphics/ColorPipeline.cpp"
)

set(BLITZAR_SERVER_SOURCES
    ${BLITZAR_ENV_UTILS_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsClientOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidation.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationPhysics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationRender.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationProfile.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveStreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/TextParse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationServer.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/CoreHelpers.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/FormatAndTheta.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/CheckpointCodec.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/PersistenceIO.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/ParseBinXyz.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/ParseVtk.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/InitialStateGeneration.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/LifecycleControls.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/RuntimeSettersAndExportStart.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/ExportAndStats.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/LoadAndCheckpoint.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/RebuildSystem.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/SnapshotAndEnergy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/TelemetryAndPendingOps.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation_server/MainLoop.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/ForceLawPolicy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/CudaMemoryPool.cu"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/cuda/ParticleSystem.cu"
)

set(BLITZAR_RUNTIME_PROTOCOL_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodec.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParse.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseStatus.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseSnapshot.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecReadNumber.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerClient.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerProtocol.cpp"
)

set(BLITZAR_CORE_FFI_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/BlitzarCore.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/BlitzarCoreOps.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/BlitzarCoreApi.cpp"
)

function(BLITZAR_collect_existing_paths out_var)
    set(_result "")
    foreach(_path IN LISTS ARGN)
        if(NOT "${_path}" STREQUAL "" AND EXISTS "${_path}")
            list(APPEND _result "${_path}")
        endif()
    endforeach()
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

if(WIN32)
    BLITZAR_collect_existing_paths(
        BLITZAR_WINDOWS_SYSTEM_INCLUDES
        "${BLITZAR_MSVC_INCLUDE_DIR}"
        "${BLITZAR_WINSDK_INCLUDE_UCRT}"
        "${BLITZAR_WINSDK_INCLUDE_UM}"
        "${BLITZAR_WINSDK_INCLUDE_SHARED}"
        "${BLITZAR_WINSDK_INCLUDE_WINRT}"
        "${BLITZAR_WINSDK_INCLUDE_CPPWINRT}"
    )
    BLITZAR_collect_existing_paths(
        BLITZAR_WINDOWS_LINK_DIRS
        "${BLITZAR_MSVC_LIB_DIR}"
        "${BLITZAR_WINSDK_UCRT_LIB_DIR}"
        "${BLITZAR_WINSDK_UM_LIB_DIR}"
    )
endif()
