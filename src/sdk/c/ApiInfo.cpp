#include "sdk/c/ApiState.hpp"

#ifndef BLITZAR_BUILD_PRODUCT_VERSION
#error "BLITZAR_BUILD_PRODUCT_VERSION must be supplied by CMake"
#endif

#ifndef BLITZAR_BUILD_PLAN_VERSION
#error "BLITZAR_BUILD_PLAN_VERSION must be supplied by CMake"
#endif

extern "C" const char* blitzar_version(void)
{
    return BLITZAR_BUILD_PRODUCT_VERSION;
}

extern "C" const char* blitzar_plan_version(void)
{
    return BLITZAR_BUILD_PLAN_VERSION;
}

extern "C" blitzar_status blitzar_get_capabilities_v2(blitzar_capabilities_v2* capabilities)
{
    if (capabilities == nullptr || !blitzar_sdk_api::HasV2Header(capabilities->struct_size,
                                       capabilities->abi_version, sizeof(*capabilities))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::uint32_t struct_size = capabilities->struct_size;
    const std::uint32_t abi_version = capabilities->abi_version;
    blitzar_compiled_backend_mask compiled_backends = BLITZAR_BACKEND_MASK_CPU;

#if defined(BLITZAR_COMPILED_HIP)

    compiled_backends |= BLITZAR_BACKEND_MASK_HIP;
#endif
#if defined(BLITZAR_COMPILED_MPI)
    compiled_backends |= BLITZAR_BACKEND_MASK_MPI;
#endif

    *capabilities = {struct_size, abi_version,
        BLITZAR_SOLVER_MASK_DIRECT | BLITZAR_SOLVER_MASK_BARNES_HUT | BLITZAR_SOLVER_MASK_FMM,
        BLITZAR_SOLVER_MASK_PM | BLITZAR_SOLVER_MASK_TREEPM,
        BLITZAR_FEATURE_GRID | BLITZAR_FEATURE_SNAPSHOT_PERSISTENCE | BLITZAR_FEATURE_HDF5 |
            BLITZAR_FEATURE_GPU_FMM | BLITZAR_FEATURE_MULTI_NODE_QUALIFICATION,
        compiled_backends};

    return BLITZAR_STATUS_OK;
}

extern "C" blitzar_status blitzar_context_status(const blitzar_context* context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return context->status;
}

extern "C" const char* blitzar_status_message(blitzar_status status)
{
    switch (status) {
    case BLITZAR_STATUS_OK:

        return "ok";

    case BLITZAR_STATUS_INVALID_ARGUMENT:

        return "invalid argument";

    case BLITZAR_STATUS_ALLOCATION_FAILURE:

        return "allocation failure";

    case BLITZAR_STATUS_INTERNAL_ERROR:

        return "internal error";

    case BLITZAR_STATUS_SINGULARITY:

        return "gravitational singularity";

    case BLITZAR_STATUS_UNSUPPORTED:

        return "unsupported";

    default:

        return "unknown status";
    }
}

extern "C" blitzar_status blitzar_simulation_status(const blitzar_simulation* simulation)
{
    if (!blitzar_sdk_api::IsValidSimulation(simulation)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.LastStatus();
}

extern "C" blitzar_status blitzar_simulation_backend(
    const blitzar_simulation* simulation, blitzar_backend_kind* backend)
{
    if (!blitzar_sdk_api::IsValidSimulation(simulation) || backend == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    *backend = simulation->implementation.LastBackend();

    return BLITZAR_STATUS_OK;
}

extern "C" blitzar_status blitzar_simulation_particle_count(
    const blitzar_simulation* simulation, std::int64_t* particle_count)
{
    if (!blitzar_sdk_api::IsValidSimulation(simulation) || particle_count == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    *particle_count = static_cast<std::int64_t>(simulation->implementation.ParticleCount());

    return BLITZAR_STATUS_OK;
}
