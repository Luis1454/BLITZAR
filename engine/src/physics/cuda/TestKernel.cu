// File: engine/src/physics/cuda/TestKernel.cu
// Purpose: Engine implementation for the BLITZAR simulation core.

#include <cstdio>
#include <cuda_runtime.h>
/// Description: Executes the minimalTestKernel operation.
__global__ void minimalTestKernel(int value)
{
    if (threadIdx.x == 0) {
        /// Description: Executes the printf operation.
        printf("[device] minimalTestKernel running with value: %d\n", value);
    }
}
extern "C" bool launchMinimalTestKernel(int value)
{
    minimalTestKernel<<<1, 32>>>(value);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        /// Description: Executes the printf operation.
        printf("[host] launchMinimalTestKernel failed: %s\n", cudaGetErrorString(err));
        return false;
    }
    /// Description: Executes the cudaDeviceSynchronize operation.
    cudaDeviceSynchronize();
    return true;
}
