# @file engine/physics/jit/Module.cmake
# @brief CUDA JIT compilation, caching, and graph specialization sources.

set(BLITZAR_PHYSICS_JIT_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/jit")
set(BLITZAR_PHYSICS_JIT_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/jit")

set(BLITZAR_PHYSICS_JIT_HOST_SOURCES)
set(BLITZAR_PHYSICS_JIT_DEVICE_SOURCES)

if(BLITZAR_ENABLE_CUDA)
    list(APPEND BLITZAR_PHYSICS_JIT_DEVICE_SOURCES
        "${BLITZAR_PHYSICS_JIT_SOURCE_DIR}/execution/CudJitRuntime.cu"
    )
else()
    list(APPEND BLITZAR_PHYSICS_JIT_HOST_SOURCES
        "${BLITZAR_PHYSICS_JIT_SOURCE_DIR}/compilation/CudJit.cpp"
    )
endif()
