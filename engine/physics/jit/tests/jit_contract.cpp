/*
 * @file engine/physics/jit/tests/jit_contract.cpp
 * @brief CUDA JIT request and fallback contract coverage.
 */

#include "physics/jit/compilation/CudJit.hpp"

#include <gtest/gtest.h>

TEST(JitTest, DefaultRequestIsStable)
{
    const CudaJitRequest request;
    EXPECT_EQ(request.family, CudaJitFamily::TreePmStencil);
    EXPECT_EQ(request.blockSize, 256);
    EXPECT_EQ(request.tileSize, 4);
}

TEST(JitTest, RuntimeReportsAStableAvailabilityState)
{
    const CudaJitRuntime runtime;
    EXPECT_EQ(runtime.available(), static_cast<bool>(BLITZAR_ENABLE_CUDA));
}
