#include <cstdio>
#include <cuda_runtime.h>
__global__ void minimalTestKernel(int value)
{
    if (threadIdx.x == 0) {
        printf("[device] minimalTestKernel running with value: %d\n", value);
    }
}
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
