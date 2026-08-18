/*
 * @file engine/physics/jit/compilation/CudJit.cpp
 * @brief No-CUDA implementation of the runtime specialization contract.
 */

#include "physics/jit/compilation/CudJit.hpp"

struct CudaJitRuntime::Impl final {};

CudaJitRuntime::CudaJitRuntime(std::string) : _impl(std::make_unique<Impl>())
{
}

CudaJitRuntime::~CudaJitRuntime() = default;

bool CudaJitRuntime::available() const noexcept
{
    return false;
}

bool CudaJitRuntime::launchTreePmNormalize(float*, float*, float*, int, float,
                                           const CudaJitRequest&, CudaJitMetrics* metrics)
{
    if (metrics != nullptr) {
        *metrics = CudaJitMetrics{};
    }
    return false;
}

bool CudaJitRuntime::launchForceTile(float*, float*, float*, float*, Vector3*, int, float, float,
                                     float, const CudaJitRequest&, CudaJitMetrics* metrics)
{
    if (metrics != nullptr) {
        *metrics = CudaJitMetrics{};
    }
    return false;
}
