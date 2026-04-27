/*
 * @file engine/src/physics/cuda/TestKernel.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

#include <cstdio>
#include <cuda_runtime.h>

/*
 * @brief Documents the minimal test kernel operation contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void minimalTestKernel(int value)
{
    if (threadIdx.x == 0) {
        printf("[device] minimalTestKernel running with value: %d\n", value);
    }
}

/*
 * @brief Documents the launch minimal test kernel operation contract.
 * @param value Input value used by this contract.
 * @return "C" bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
extern "C" bool launchMinimalTestKernel(int value)
{
    minimalTestKernel<<<1, 32>>>(value);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("[host] launchMinimalTestKernel failed: %s\n", cudaGetErrorString(err));
        return false;
    }
    cudaDeviceSynchronize();
    return true;
}
