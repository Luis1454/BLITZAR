/*
 * @file engine/config/registry/CfgApplyNormalized.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Canonicalization and preset application for registry entries.
 */

#include "CfgInternal.hpp"

#include "config/core/CfgConfig.hpp"
#include "config/modes/CfgNormalize.hpp"
#include "config/profile/CfgPerformance.hpp"

#include <ostream>

namespace bltzr_config {

template <typename ValueType>
static ValueType& memberAt(SimulationConfig& config, std::ptrdiff_t offset)
{
    return *reinterpret_cast<ValueType*>(reinterpret_cast<char*>(&config) + offset);
}

bool applyNormalizedEntry(const SimulationOptionEntry& entry, const std::string& value,
                          SimulationConfig& config, std::ostream& warnings, std::string_view source,
                          std::string_view optionName)
{
    switch (entry.kind) {
    case OptionKind::PerformanceProfile: {
        std::string canonical;
        if (!normalizePerformanceProfile(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: interactive|balanced|quality|custom)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        applyPerformanceProfile(config);
        return true;
    }
    case OptionKind::Solver: {
        std::string canonical;
        if (!bltzr_modes::normalizeSolver(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::Integrator: {
        std::string canonical;
        if (!bltzr_modes::normalizeIntegrator(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: euler|rk4|leapfrog)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::OctreeCriterion: {
        std::string canonical;
        if (!bltzr_modes::normalizeOctreeOpeningCriterion(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: com|bounds)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::TreePmModel: {
        std::string canonical;
        if (!normalizeTreePmModel(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: auto|local_grid|tree|exact_tree|hybrid|pm_only)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::TreePmLayout: {
        std::string canonical;
        if (!normalizeTreePmLayout(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: auto|linear|gather_linear|gather_morton)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::TreePmPrecision: {
        std::string canonical;
        if (!normalizeTreePmPrecision(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: fp32|fp64)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::TreePmAssignment: {
        std::string canonical;
        if (!normalizeTreePmAssignment(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: cic|tsc|pcs)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::TreePmPreset: {
        std::string canonical;
        if (!normalizeTreePmPreset(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: pm_only|local_grid_fast|hybrid_balanced|hybrid_quality|"
                        "tree_quality|custom)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        applyTreePmPreset(config);
        return true;
    }
    default:
        return false;
    }
}

} // namespace bltzr_config
