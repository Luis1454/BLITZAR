# @file engine/physics/cuda/Module.cmake
# @brief CUDA runtime, memory, and JIT bridge sources.

set(BLITZAR_PHYSICS_CUDA_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda")
set(BLITZAR_PHYSICS_CUDA_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda")

set(BLITZAR_PHYSICS_CUDA_HOST_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudDeviceMemory.cpp"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudJit.cpp"
)

set(BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudDeviceMemory.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudMemoryPool.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudJitRuntime.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/CudParticleSystem.cu"
)
