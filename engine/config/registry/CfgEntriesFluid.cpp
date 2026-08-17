/*
 * @file engine/config/registry/CfgEntriesFluid.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused simulation option definitions.
 */

#include "CfgInternal.hpp"

#include "config/core/CfgConfig.hpp"

#include <cstddef>

namespace bltzr_config {

extern const SimulationOptionEntry kFluidPrimaryOptions[] = {
    {SimulationOptionGroup::Fluid, OptionKind::Bool, "--sph", "", "sph_enabled", "", "",
     "  --sph <true|false>\n", "", offsetof(SimulationConfig, sphEnabled), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-h", "", "sph_smoothing_length", "", "",
     "  --sph-h <float>\n", "", offsetof(SimulationConfig, sphSmoothingLength), 0.05, 0.0, true,
     false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-rest-density", "", "sph_rest_density",
     "", "", "  --sph-rest-density <float>\n", "", offsetof(SimulationConfig, sphRestDensity), 0.01,
     0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-gas-constant", "", "sph_gas_constant",
     "", "", "  --sph-gas-constant <float>\n", "", offsetof(SimulationConfig, sphGasConstant), 0.01,
     0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-viscosity", "", "sph_viscosity", "",
     "", "  --sph-viscosity <float>\n", "", offsetof(SimulationConfig, sphViscosity), 0.0, 0.0,
     true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Uint, "--energy-every", "",
     "energy_measure_every_steps", "", "", "  --energy-every <n>\n", "",
     offsetof(SimulationConfig, energyMeasureEverySteps), 1.0, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Uint, "--energy-sample-limit", "",
     "energy_sample_limit", "", "", "  --energy-sample-limit <n>\n", "",
     offsetof(SimulationConfig, energySampleLimit), 64.0, 0.0, true, false},
};

extern const std::size_t kFluidPrimaryOptionCount =
    sizeof(kFluidPrimaryOptions) / sizeof(kFluidPrimaryOptions[0]);

extern const SimulationOptionEntry kFluidTailOptions[] = {
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-max-accel", "", "sph_max_acceleration",
     "", "", "  --sph-max-accel <float>\n", "", offsetof(SimulationConfig, sphMaxAcceleration), 0.0,
     0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-max-speed", "", "sph_max_speed", "",
     "", "  --sph-max-speed <float>\n", "", offsetof(SimulationConfig, sphMaxSpeed), 0.0, 0.0, true,
     false},
};

extern const std::size_t kFluidTailOptionCount =
    sizeof(kFluidTailOptions) / sizeof(kFluidTailOptions[0]);

} // namespace bltzr_config
