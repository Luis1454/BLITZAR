/*
 * @file modules/qt/ui/ThroughputAdvisor.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/ThroughputAdvisor.hpp"
#include "config/SimulationConfig.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace bltzr_qt {
class ThroughputAdvisorLocal final {
public:
    static std::uint32_t estimateSubsteps(const SimulationConfig& config)
    {
        if (config.substepTargetDt <= 0.0f || config.dt <= 0.0f)
            return 1u;
        const float raw = std::ceil(config.dt / config.substepTargetDt);
        return std::max(1u, std::min(config.maxSubsteps, static_cast<std::uint32_t>(raw)));
    }

    static float solverPenalty(const SimulationConfig& config, std::uint32_t substeps)
    {
        const float particles = static_cast<float>(std::max(2u, config.particleCount));
        const float substepPenalty = static_cast<float>(std::max(1u, substeps));
        if (config.solver == "pairwise_cuda") {
            const float normalized = particles / 20000.0f;
            return std::max(1.0f, normalized * normalized) * substepPenalty;
        }
        if (config.solver == "octree_cpu")
            return std::max(1.0f, particles / 50000.0f) * substepPenalty * 1.5f;
        return std::max(1.0f, particles / 200000.0f) * substepPenalty;
    }

    static float drawPenalty(const SimulationConfig& config, std::uint32_t drawCap)
    {
        const std::uint32_t effectiveCap =
            std::max<std::uint32_t>(2u, drawCap == 0u ? config.clientParticleCap : drawCap);
        return std::max(1.0f, static_cast<float>(effectiveCap) / 50000.0f);
    }

    static std::string suggestedAction(const SimulationConfig& config, std::uint32_t substeps,
                                       std::uint32_t drawCap)
    {
        std::ostringstream out;
        bool first = true;
        const auto append = [&out, &first](const std::string& text) {
            if (!first) {
                out << "; ";
            }
            out << text;
            first = false;
        };
        if (config.solver == "pairwise_cuda" && config.particleCount > 20000u) {
            append("switch solver to octree_gpu");
        }
        if (substeps > 2u) {
            append("reduce dt or relax substep_target_dt");
        }
        if (config.clientParticleCap > 20000u || drawCap >= 20000u) {
            append("lower draw cap");
        }
        if (config.particleCount > 50000u) {
            append("reduce particle_count or use a lighter preset");
        }
        if (first) {
            append("use the balanced or interactive run profile");
        }
        return out.str();
    }
};

ThroughputAdvisory ThroughputAdvisor::evaluate(const SimulationConfig& config,
                                               std::uint32_t drawCap)
{
    ThroughputAdvisory advisory;
    advisory.estimatedSubsteps = ThroughputAdvisorLocal::estimateSubsteps(config);
    const float penalty =
        ThroughputAdvisorLocal::solverPenalty(config, advisory.estimatedSubsteps) *
        ThroughputAdvisorLocal::drawPenalty(config, drawCap);
    advisory.estimatedStepsPerSecond = 30.0f / std::max(1.0f, penalty);
    if (advisory.estimatedStepsPerSecond >= 5.0f)
        return advisory;
    advisory.severity = advisory.estimatedStepsPerSecond < 1.0f
                            ? ThroughputAdvisorySeverity::Warning
                            : ThroughputAdvisorySeverity::Advisory;
    std::ostringstream summary;
    summary.setf(std::ios::fixed);
    summary.precision(1);
    summary << (advisory.severity == ThroughputAdvisorySeverity::Warning ? "Throughput warning"
                                                                         : "Throughput advisory")
            << ": estimated " << advisory.estimatedStepsPerSecond
            << " step/s with solver=" << config.solver << ", particles=" << config.particleCount
            << ", substeps=" << advisory.estimatedSubsteps << ", draw_cap="
            << std::max<std::uint32_t>(2u, drawCap == 0u ? config.clientParticleCap : drawCap);
    advisory.summary = summary.str();
    advisory.action =
        ThroughputAdvisorLocal::suggestedAction(config, advisory.estimatedSubsteps, drawCap);
    advisory.statusBarText = advisory.summary + ". Try: " + advisory.action + ".";
    return advisory;
}
} // namespace bltzr_qt
