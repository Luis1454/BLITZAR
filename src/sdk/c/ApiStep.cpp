#include "sdk/c/ApiState.hpp"

extern "C" blitzar_status blitzar_simulation_step(blitzar_simulation* simulation)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.Step();
}
