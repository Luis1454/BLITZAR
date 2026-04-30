/*
 * @file modules/qt/ui/MainWindowController.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/MainWindowController.hpp"
#include "client/ClientCommon.hpp"
#include "config/SimulationConfig.hpp"
#include "server/SimulationInitConfig.hpp"
#include <iostream>

namespace bltzr_qt {
class MainWindowControllerLocal final {
public:
    static void applySharedConfig(const SimulationConfig& config,
                                  bltzr_client::IClientRuntime& runtime)
    {
        runtime.setParticleCount(bltzr_client::resolveServerParticleCount(config));
        runtime.setDt(config.dt);
        runtime.setSolverMode(config.solver);
        runtime.setIntegratorMode(config.integrator);
        runtime.setPerformanceProfile(config.performanceProfile);
        runtime.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
        runtime.setSphEnabled(config.sphEnabled);
        runtime.setSphParameters(config.sphSmoothingLength, config.sphRestDensity,
                                 config.sphGasConstant, config.sphViscosity);
        runtime.setSubstepPolicy(config.substepTargetDt, config.maxSubsteps);
        runtime.setSnapshotPublishPeriodMs(config.snapshotPublishPeriodMs);
        runtime.setEnergyMeasurementConfig(config.energyMeasureEverySteps,
                                           config.energySampleLimit);
        runtime.setExportDefaults(config.exportDirectory, config.exportFormat);
        runtime.setRemoteSnapshotCap(bltzr_client::resolveClientDrawCap(config));
    }
};

MainWindowApplyConfigResult MainWindowController::applyConfig(const SimulationConfig& config,
                                                              bltzr_client::IClientRuntime& runtime,
                                                              bool requestReset) const
{
    MainWindowApplyConfigResult result;
    result.report = validate(config);
    result.clientDrawCap = bltzr_client::resolveClientDrawCap(config);
    if (!result.report.validForRun)
        return result;
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(config, std::cerr);
    MainWindowControllerLocal::applySharedConfig(config, runtime);
    runtime.setInitialStateFile(initPlan.inputFile, initPlan.inputFormat);
    runtime.setInitialStateConfig(initPlan.config);
    if (requestReset) {
        runtime.requestReset();
    }
    result.applied = true;
    return result;
}

std::uint32_t MainWindowController::applyPerformanceProfile(const SimulationConfig& config,
                                              bltzr_client::IClientRuntime& runtime) const
{
    MainWindowControllerLocal::applySharedConfig(config, runtime);
    return bltzr_client::resolveClientDrawCap(config);
}

bltzr_config::ScenarioValidationReport MainWindowController::validate(const SimulationConfig& config) const
{
    return bltzr_config::SimulationScenarioValidation::evaluate(config);
}
} // namespace bltzr_qt
