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
set(BLITZAR_CUDA_ARCHITECTURES "75-real;80-real;86-real;89-real" CACHE STRING
    "CUDA architectures emitted by BLITZAR targets; use one local GPU architecture for low-memory development builds")
set(CMAKE_CUDA_ARCHITECTURES "${BLITZAR_CUDA_ARCHITECTURES}")
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

include("${BLITZAR_ROOT_DIR}/engine/Module.cmake")

set(BLITZAR_PROJECT_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/client-host/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/server-service/include"
    ${BLITZAR_ENGINE_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/server"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/qt"
)

set(BLITZAR_BATCH_COMMON_SOURCES
    ${BLITZAR_BATCH_MODULE_SOURCES}
    ${BLITZAR_CONFIG_SOURCES}
    ${BLITZAR_SERVER_MODULE_SOURCES}
    ${BLITZAR_PHYSICS_CORE_SOURCES}
    ${BLITZAR_PHYSICS_TREEPM_SOURCES}
    ${BLITZAR_PHYSICS_FMM_SOURCES}
    ${BLITZAR_PHYSICS_JIT_HOST_SOURCES}
)

set(BLITZAR_SERVER_COMMON_SOURCES
    ${BLITZAR_BATCH_COMMON_SOURCES}
    ${BLITZAR_SERVER_RUNTIME_SOURCES}
)

if(BLITZAR_ENABLE_CUDA)
    set(BLITZAR_BATCH_SOURCES
        ${BLITZAR_BATCH_COMMON_SOURCES}
        ${BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES}
        ${BLITZAR_PHYSICS_JIT_DEVICE_SOURCES}
    )
    set(BLITZAR_SERVER_SOURCES
        ${BLITZAR_SERVER_COMMON_SOURCES}
        ${BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES}
        ${BLITZAR_PHYSICS_JIT_DEVICE_SOURCES}
    )
else()
    set(BLITZAR_BATCH_SOURCES
        ${BLITZAR_BATCH_COMMON_SOURCES}
        ${BLITZAR_PHYSICS_CUDA_HOST_SOURCES}
        ${BLITZAR_PHYSICS_JIT_HOST_SOURCES}
        ${BLITZAR_PHYSICS_OCTREE_SOURCES}
    )
    set(BLITZAR_SERVER_SOURCES
        ${BLITZAR_SERVER_COMMON_SOURCES}
        ${BLITZAR_PHYSICS_CUDA_HOST_SOURCES}
        ${BLITZAR_PHYSICS_JIT_HOST_SOURCES}
        ${BLITZAR_PHYSICS_OCTREE_SOURCES}
    )
endif()

set(BLITZAR_RUNTIME_PROTOCOL_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/PtcJsonCodec.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcParser.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcStatus.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcSnapshot.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcNumber.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/client/PtcClient.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/protocol/PtcProtocol.cpp"
)

set(BLITZAR_CORE_FFI_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/ffi/core/FfiCore.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/ffi/core/FfiOps.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/ffi/core/FfiApi.cpp"
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
