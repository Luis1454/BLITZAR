# @file cmake/core/toolchain.cmake
# @author BLITZAR Contributors
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
find_package(CUDAToolkit QUIET)

set(BLITZAR_PROJECT_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/client-host/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/server-service/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/qt/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/physics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/include/graphics"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/src/cuda"
)

if(WIN32)
    set(BLITZAR_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Win.cpp"
    )
else()
    set(BLITZAR_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Posix.cpp"
    )
endif()

set(BLITZAR_GRAPHICS_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/src/graphics/ViewMath.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/graphics/ColorPipeline.cpp"
)

set(BLITZAR_SERVER_COMMON_SOURCES
    ${BLITZAR_ENV_UTILS_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/Parse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/CoreOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/ClientOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/InitOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/InitStateOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/FluidOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Apply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Entries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Performance.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Scenario.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Physics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Render.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Config.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Write.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/StreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/ValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/core/Config.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/modes/Normalize.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/text/Parse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/core/Helpers.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/core/FormatAndTheta.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/persistence/Checkpoint.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/persistence/IO.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/parsing/BinXyz.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/parsing/Vtk.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/state/Generation.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/lifecycle/Controls.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/runtime/SettersAndExport.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/export/Stats.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/persistence/LoadAndCheckpoint.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/lifecycle/Rebuild.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/telemetry/SnapshotAndEnergy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/telemetry/PendingOps.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/simulation/lifecycle/Loop.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/ForceLawPolicy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/ParticleHotData.cpp"
)

if(BLITZAR_ENABLE_CUDA)
    set(BLITZAR_SERVER_SOURCES
        ${BLITZAR_SERVER_COMMON_SOURCES}
        "${BLITZAR_ROOT_DIR}/engine/src/cuda/MemoryPool.cu"
        "${BLITZAR_ROOT_DIR}/engine/src/cuda/ParticleSystem.cu"
    )
else()
    set(BLITZAR_SERVER_SOURCES
        ${BLITZAR_SERVER_COMMON_SOURCES}
        "${BLITZAR_ROOT_DIR}/engine/src/physics/ParticleSystemHost.cpp"
    )
endif()

set(BLITZAR_RUNTIME_PROTOCOL_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/JsonCodec.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Parser.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Status.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Snapshot.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Number.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/client/Client.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/protocol/Protocol.cpp"
)

set(BLITZAR_CORE_FFI_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/core/Core.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/core/Ops.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/ffi/core/Api.cpp"
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
