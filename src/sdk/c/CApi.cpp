#include "sdk/c/CApiState.hpp"

#include <new>
#include <stdexcept>

#ifndef BLITZAR_BUILD_PRODUCT_VERSION
#error "BLITZAR_BUILD_PRODUCT_VERSION must be supplied by CMake"
#endif

#ifndef BLITZAR_BUILD_PLAN_VERSION
#error "BLITZAR_BUILD_PLAN_VERSION must be supplied by CMake"
#endif

extern "C" blitzar_status blitzar_context_create(blitzar_context** context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    *context = nullptr;

    try {
        *context = new blitzar_context{BLITZAR_STATUS_OK};
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

extern "C" void blitzar_context_destroy(blitzar_context* context)
{
    delete context;
}

extern "C" blitzar_status blitzar_simulation_create(
    blitzar_context* context, std::int64_t particle_count, blitzar_simulation** simulation)
{
    if (context == nullptr || context->status != BLITZAR_STATUS_OK || simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    *simulation = nullptr;

    std::size_t converted_count = 0;

    if (!blitzar_sdk_api::TryConvertCount(particle_count, converted_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        *simulation = new blitzar_simulation(converted_count);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

extern "C" void blitzar_simulation_destroy(blitzar_simulation* simulation)
{
    delete simulation;
}
