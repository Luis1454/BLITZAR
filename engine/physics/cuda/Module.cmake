# @file engine/physics/cuda/Module.cmake
# @brief Shared CUDA runtime, memory, and integration sources.

set(BLITZAR_PHYSICS_CUDA_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda")
set(BLITZAR_PHYSICS_CUDA_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/cuda")

set(BLITZAR_PHYSICS_CUDA_HOST_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudDeviceMemory.cpp"
)

set(BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudDeviceMemory.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudMemoryPool.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/runtime/CudParticleSystem.cu"
)
