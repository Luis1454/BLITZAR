/*
 * @file engine/server/simulation/state/SrvAtomic.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Atomic floating-point utility used by simulation telemetry.
 */

#include "server/simulation/runtime/SrvInternal.hpp"

void atomicAddFloat(std::atomic<float>& atom, float value)
{
    float current = atom.load(std::memory_order_relaxed);
    while (!atom.compare_exchange_weak(current, current + value, std::memory_order_relaxed)) {
        // current is updated with the latest value on failure.
    }
}
