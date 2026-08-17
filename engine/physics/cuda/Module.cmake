# @file engine/physics/cuda/Module.cmake
# @brief CUDA runtime, memory, and JIT bridge sources.

set(BLITZAR_PHYSICS_CUDA_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda/include")
set(BLITZAR_PHYSICS_CUDA_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda/src")

set(BLITZAR_PHYSICS_CUDA_HOST_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/DeviceMemory.cpp"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudaJit.cpp"
)

set(BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/DeviceMemory.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudMemoryPool.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/JitRuntime.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/ParticleSystem.cu"
)
