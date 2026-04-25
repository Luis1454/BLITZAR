set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED GRAVITY_ROOT_DIR)
    get_filename_component(GRAVITY_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
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

gravity_populate_windows_toolchain_hints()
find_package(CUDAToolkit REQUIRED)

set(GRAVITY_PROJECT_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/qt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/physics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/graphics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/physics/cuda/fragments"
)

if(WIN32)
    set(GRAVITY_ENV_UTILS_SOURCES
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(GRAVITY_ENV_UTILS_SOURCES
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()

set(GRAVITY_GRAPHICS_SOURCES
    "${GRAVITY_ROOT_DIR}/engine/src/graphics/ViewMath.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/graphics/ColorPipeline.cpp"
)

set(GRAVITY_SERVER_SOURCES
    ${GRAVITY_ENV_UTILS_SOURCES}
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsClientOptions.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationScenarioValidation.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationScenarioValidationPhysics.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationScenarioValidationRender.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationProfile.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/DirectiveStreamWriter.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/DirectiveValueFormatter.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationServer.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/CoreHelpers.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/FormatAndTheta.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/CheckpointCodec.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/PersistenceIO.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/ParseBinXyz.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/ParseVtk.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/InitialStateGeneration.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/LifecycleControls.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/RuntimeSettersAndExportStart.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/ExportAndStats.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/LoadAndCheckpoint.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/RebuildSystem.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/SnapshotAndEnergy.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/TelemetryAndPendingOps.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/simulation_server/MainLoop.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/physics/ForceLawPolicy.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/physics/CudaMemoryPool.cu"
    "${GRAVITY_ROOT_DIR}/engine/src/physics/cuda/ParticleSystem.cu"
)

set(GRAVITY_RUNTIME_PROTOCOL_SOURCES
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodec.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParse.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseStatus.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseSnapshot.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecReadNumber.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerClient.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerProtocol.cpp"
)

set(GRAVITY_CORE_FFI_SOURCES
    "${GRAVITY_ROOT_DIR}/runtime/src/ffi/BlitzarCore.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/ffi/BlitzarCoreOps.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/ffi/BlitzarCoreApi.cpp"
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
