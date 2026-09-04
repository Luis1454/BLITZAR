#include "io/metadata/MetadataFields.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

namespace blitzar_io {

namespace {

[[nodiscard]] bool ReadMetadataSimulation(MetadataCursor& cursor, MetadataSimulation& simulation)
{
    std::uint64_t solver{};
    std::uint64_t integrator{};

    if (!cursor.Expect("    \"simulation\": {") ||
        !cursor.ReadUnsigned("      \"particle_count\": ", ",", simulation.particle_count) ||
        !cursor.ReadUnsigned("      \"requested_steps\": ", ",", simulation.requested_steps) ||
        !cursor.ReadReal("      \"timestep\": ", ",", simulation.timestep) ||
        !cursor.ReadUnsigned("      \"solver\": ", ",", solver) ||
        !cursor.ReadUnsigned("      \"integrator\": ", "", integrator) ||
        solver > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        integrator > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        !cursor.Expect("    },")) {
        return false;
    }

    simulation.solver = static_cast<blitzar_solver_kind>(solver);
    simulation.integrator = static_cast<blitzar_integrator_kind>(integrator);

    return true;
}

[[nodiscard]] bool ReadMetadataGravity(MetadataCursor& cursor, MetadataGravity& gravity)
{
    return cursor.Expect("    \"gravity\": {") &&
           cursor.ReadReal(
               "      \"gravitational_constant\": ", ",", gravity.gravitational_constant) &&
           cursor.ReadReal("      \"softening\": ", "", gravity.softening) &&
           cursor.Expect("    },");
}

[[nodiscard]] bool ReadMetadataUnits(MetadataCursor& cursor, MetadataUnits& units)
{
    return cursor.Expect("    \"units\": {") &&
           cursor.ReadReal("      \"length_scale\": ", ",", units.length_scale) &&
           cursor.ReadReal("      \"mass_scale\": ", ",", units.mass_scale) &&
           cursor.ReadReal("      \"time_scale\": ", "", units.time_scale) &&
           cursor.Expect("    },");
}

[[nodiscard]] bool ReadMetadataBarnesHut(MetadataCursor& cursor, MetadataBarnesHut& barnes_hut)
{
    return cursor.Expect("    \"barnes_hut\": {") &&
           cursor.ReadReal("      \"opening_angle\": ", ",", barnes_hut.opening_angle) &&
           cursor.ReadUnsigned("      \"max_particles\": ", ",", barnes_hut.max_particles) &&
           cursor.ReadUnsigned("      \"max_cells\": ", ",", barnes_hut.max_cells) &&
           cursor.ReadUnsigned("      \"leaf_capacity\": ", ",", barnes_hut.leaf_capacity) &&
           cursor.ReadUnsigned("      \"max_depth\": ", "", barnes_hut.max_depth) &&
           cursor.Expect("    },");
}

[[nodiscard]] bool ReadMetadataGeneration(MetadataCursor& cursor, MetadataGeneration& generation)
{
    return cursor.Expect("    \"generation\": {") &&
           cursor.ReadUnsigned("      \"seed\": ", ",", generation.seed) &&
           cursor.ReadBoolean("      \"deterministic\": ", "", generation.deterministic) &&
           cursor.Expect("    },");
}

[[nodiscard]] bool ReadMetadataBackendExecutionPolicy(
    MetadataCursor& cursor, std::string_view name, blitzar_core::BackendExecutionPolicy& policy)
{
    std::string fma;
    std::string reduction;
    blitzar_core::FmaPolicy parsed_fma{};
    blitzar_core::ReductionPolicy parsed_reduction{};

    if (!cursor.Expect(std::string{"      \""} + std::string{name} + "\": {") ||

        !cursor.ReadString("        \"fma\": ", ",", fma) ||
        !cursor.ReadString("        \"reduction\": ", "", reduction) ||
        !blitzar_core::ParseFmaPolicy(fma, parsed_fma) ||
        !blitzar_core::ParseReductionPolicy(reduction, parsed_reduction) ||
        !cursor.Expect("      },")) {
        return false;
    }

    policy = {parsed_fma, parsed_reduction};

    return true;
}

[[nodiscard]] bool ReadMetadataExecution(MetadataCursor& cursor, MetadataExecution& execution)
{
    std::string mode;
    std::string precision;
    std::string compiler;
    std::string device;
    std::string rng;
    std::string compensator;
    std::string ordering;
    bool bitwise_reproducible = false;
    blitzar_core::ExecutionMode parsed_mode{};

    if (!cursor.Expect("    \"execution\": {") ||
        !cursor.ReadString("      \"mode\": ", ",", mode) ||
        !blitzar_core::ParseExecutionMode(mode, parsed_mode) ||
        !ReadMetadataBackendExecutionPolicy(cursor, "cpu", execution.cpu) ||
        !ReadMetadataBackendExecutionPolicy(cursor, "hip", execution.hip) ||
        !ReadMetadataBackendExecutionPolicy(cursor, "mpi", execution.mpi) ||
        !cursor.ReadString("      \"precision\": ", ",", precision) ||
        !cursor.ReadString("      \"compiler\": ", ",", compiler) ||
        !cursor.ReadString("      \"device\": ", ",", device) ||
        !cursor.ReadString("      \"rng\": ", ",", rng) ||
        !cursor.ReadString("      \"compensator\": ", ",", compensator) ||
        !cursor.ReadString("      \"ordering\": ", ",", ordering) ||
        !cursor.ReadBoolean("      \"bitwise_reproducible\": ", "", bitwise_reproducible) ||
        !cursor.Expect("    },")) {
        return false;
    }

    execution.mode = parsed_mode;
    execution.precision = std::move(precision);
    execution.compiler = std::move(compiler);
    execution.device = std::move(device);
    execution.rng = std::move(rng);
    execution.compensator = std::move(compensator);
    execution.ordering = std::move(ordering);
    execution.bitwise_reproducible = bitwise_reproducible;

    return true;
}

[[nodiscard]] bool ReadMetadataOutput(MetadataCursor& cursor, MetadataOutput& output)
{
    std::string format;

    return cursor.Expect("    \"output\": {") &&
           cursor.ReadBoolean("      \"enabled\": ", ",", output.enabled) &&
           cursor.ReadString("      \"format\": ", ",", format) &&
           cursor.ReadUnsigned("      \"every_steps\": ", ",", output.every_steps) &&
           cursor.ReadBoolean("      \"write_initial\": ", ",", output.write_initial) &&
           cursor.ReadBoolean("      \"write_final\": ", "", output.write_final) &&
           ParseMetadataOutputFormat(format, output.format) && cursor.Expect("    },");
}

[[nodiscard]] bool ReadMetadataDiagnostics(MetadataCursor& cursor, MetadataDiagnostics& diagnostics)
{
    return cursor.Expect("    \"diagnostics\": {") &&
           cursor.ReadBoolean("      \"enabled\": ", ",", diagnostics.enabled) &&
           cursor.ReadUnsigned("      \"every_steps\": ", ",", diagnostics.every_steps) &&
           cursor.ReadBoolean("      \"energy\": ", ",", diagnostics.energy) &&
           cursor.ReadBoolean("      \"momentum\": ", ",", diagnostics.momentum) &&
           cursor.ReadBoolean("      \"relative_error\": ", "", diagnostics.relative_error) &&
           cursor.Expect("    }");
}

} // namespace

bool ReadMetadataConfiguration(MetadataCursor& cursor, MetadataRunConfiguration& configuration)
{
    return cursor.Expect("  \"configuration\": {") &&
           ReadMetadataSimulation(cursor, configuration.simulation) &&
           ReadMetadataGravity(cursor, configuration.gravity) &&
           ReadMetadataUnits(cursor, configuration.units) &&
           ReadMetadataBarnesHut(cursor, configuration.barnes_hut) &&
           ReadMetadataGeneration(cursor, configuration.generation) &&
           ReadMetadataExecution(cursor, configuration.execution) &&
           ReadMetadataOutput(cursor, configuration.output) &&
           ReadMetadataDiagnostics(cursor, configuration.diagnostics) && cursor.Expect("  },");
}

bool ReadMetadataCapabilities(MetadataCursor& cursor, MetadataCapabilities& capabilities)
{
    std::uint64_t implemented{};
    std::uint64_t unsupported{};
    std::uint64_t deferred{};
    std::uint64_t compiled{};

    if (!cursor.Expect("  \"capabilities\": {") ||
        !cursor.ReadUnsigned("    \"implemented_solver_mask\": ", ",", implemented) ||
        !cursor.ReadUnsigned("    \"unsupported_solver_mask\": ", ",", unsupported) ||
        !cursor.ReadUnsigned("    \"deferred_feature_mask\": ", ",", deferred) ||
        !cursor.ReadUnsigned("    \"compiled_backend_mask\": ", "", compiled) ||
        implemented > std::numeric_limits<std::uint32_t>::max() ||
        unsupported > std::numeric_limits<std::uint32_t>::max() ||
        deferred > std::numeric_limits<std::uint32_t>::max() ||
        compiled > std::numeric_limits<std::uint32_t>::max() || !cursor.Expect("  },")) {
        return false;
    }

    capabilities.implemented_solver_mask = static_cast<blitzar_solver_mask>(implemented);
    capabilities.unsupported_solver_mask = static_cast<blitzar_solver_mask>(unsupported);
    capabilities.deferred_feature_mask = static_cast<blitzar_feature_mask>(deferred);
    capabilities.compiled_backend_mask = static_cast<blitzar_compiled_backend_mask>(compiled);

    return true;
}

bool ReadMetadataDistribution(MetadataCursor& cursor, MetadataRunInfo& info)
{
    std::uint64_t rank_count{};
    std::uint64_t rank_index{};

    if (!cursor.Expect("  \"distribution\": {") ||
        !cursor.ReadUnsigned("    \"rank_count\": ", ",", rank_count) ||
        !cursor.ReadUnsigned("    \"rank_index\": ", "", rank_index) ||
        rank_count > std::numeric_limits<std::uint32_t>::max() ||
        rank_index > std::numeric_limits<std::uint32_t>::max() || !cursor.Expect("  },")) {
        return false;
    }

    info.rank_count = static_cast<std::uint32_t>(rank_count);
    info.rank_index = static_cast<std::uint32_t>(rank_index);

    return true;
}

bool ReadMetadataOutputs(MetadataCursor& cursor, MetadataRunInfo& info,
    std::uint64_t expected_count, std::vector<std::uint64_t>& completed_steps)
{
    if (info.rank_count == 0U || info.rank_count > MetadataMaxRankIndex + 1U ||
        expected_count > MetadataMaxStepCount + 1U ||
        !cursor.Expect("  \"completed_outputs\": [")) {
        return false;
    }

    completed_steps.clear();
    completed_steps.reserve(static_cast<std::size_t>(expected_count));

    for (std::size_t index = 0; index < static_cast<std::size_t>(expected_count); ++index) {
        std::uint64_t step{};

        if (!cursor.Expect("    {") || !cursor.ReadUnsigned("      \"step\": ", ",", step)) {
            return false;
        }

        if (info.rank_count == 1U) {
            std::string path;
            const std::string expected_path =
                "states/" + StateFileName(step, info.configuration.output.format);

            if (expected_path == "states/" || !cursor.ReadString("      \"path\": ", "", path) ||
                path != expected_path) {
                return false;
            }
        }
        else {
            if (!cursor.Expect("      \"shards\": [")) {
                return false;
            }

            for (std::uint32_t rank = 0; rank < info.rank_count; ++rank) {
                std::string path;
                const std::string expected_path =
                    "states/" + StateShardFileName(step, rank, info.configuration.output.format);

                const std::string separator = rank + 1U == info.rank_count ? "" : ",";

                if (expected_path == "states/" || !cursor.ReadString("        ", separator, path) ||
                    path != expected_path) {
                    return false;
                }
            }

            if (!cursor.Expect("      ]")) {
                return false;
            }
        }

        const std::string closing =
            index + 1U == static_cast<std::size_t>(expected_count) ? "    }" : "    },";

        if (!cursor.Expect(closing)) {
            return false;
        }

        completed_steps.push_back(step);
    }

    return cursor.Expect("  ]");
}

bool HasValidMetadataSteps(
    const std::vector<std::uint64_t>& completed_steps, std::uint64_t requested_steps) noexcept
{
    std::uint64_t previous{};
    bool has_previous = false;

    for (const std::uint64_t step : completed_steps) {
        if (step > requested_steps || step > MetadataMaxStateStep ||
            (has_previous && step <= previous)) {
            return false;
        }

        previous = step;
        has_previous = true;
    }

    return true;
}

} // namespace blitzar_io
