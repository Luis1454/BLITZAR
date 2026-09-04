#include "io/metadata/MetadataManifest.hpp"

#include <iomanip>
#include <locale>
#include <ostream>
#include <string_view>

namespace blitzar_io {

namespace {

[[nodiscard]] bool WriteJsonString(std::ostream& output, std::string_view value)
{
    output.put('"');

    for (const unsigned char character : value) {
        switch (character) {
        case '"':

            output << "\\\"";

            break;

        case '\\':

            output << "\\\\";

            break;

        case '\b':

            output << "\\b";

            break;

        case '\f':

            output << "\\f";

            break;

        case '\n':

            output << "\\n";

            break;

        case '\r':

            output << "\\r";

            break;

        case '\t':

            output << "\\t";

            break;

        default:

            if (character < 0x20U) {
                constexpr char Hex[] = "0123456789abcdef";

                output << "\\u00" << Hex[(character >> 4U) & 0x0fU] << Hex[character & 0x0fU];
            }
            else {
                output.put(static_cast<char>(character));
            }

            break;
        }
    }

    output.put('"');

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteSimulation(std::ostream& output, const MetadataSimulation& simulation)
{
    output << "    \"simulation\": {\n"
              "      \"particle_count\": "
           << simulation.particle_count
           << ",\n"
              "      \"requested_steps\": "
           << simulation.requested_steps
           << ",\n"
              "      \"timestep\": "
           << simulation.timestep
           << ",\n"
              "      \"solver\": "
           << static_cast<std::int32_t>(simulation.solver)
           << ",\n"
              "      \"integrator\": "
           << static_cast<std::int32_t>(simulation.integrator)
           << "\n"
              "    },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteGravity(std::ostream& output, const MetadataGravity& gravity)
{
    output << "    \"gravity\": {\n"
              "      \"gravitational_constant\": "
           << gravity.gravitational_constant
           << ",\n"
              "      \"softening\": "
           << gravity.softening
           << "\n"
              "    },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteUnits(std::ostream& output, const MetadataUnits& units)
{
    output << "    \"units\": {\n"
              "      \"length_scale\": "
           << units.length_scale
           << ",\n"
              "      \"mass_scale\": "
           << units.mass_scale
           << ",\n"
              "      \"time_scale\": "
           << units.time_scale
           << "\n"
              "    },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteBarnesHut(std::ostream& output, const MetadataBarnesHut& barnes_hut)
{
    output << "    \"barnes_hut\": {\n"
              "      \"opening_angle\": "
           << barnes_hut.opening_angle
           << ",\n"
              "      \"max_particles\": "
           << barnes_hut.max_particles
           << ",\n"
              "      \"max_cells\": "
           << barnes_hut.max_cells
           << ",\n"
              "      \"leaf_capacity\": "
           << barnes_hut.leaf_capacity
           << ",\n"
              "      \"max_depth\": "
           << barnes_hut.max_depth
           << "\n"
              "    },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteOutput(std::ostream& output, const MetadataOutput& settings)
{
    output << "    \"output\": {\n"
              "      \"enabled\": "
           << (settings.enabled ? "true" : "false")
           << ",\n"
              "      \"format\": ";

    if (!WriteJsonString(output, MetadataOutputFormatName(settings.format))) {
        return false;
    }

    output << ",\n"
              "      \"every_steps\": "
           << settings.every_steps
           << ",\n"
              "      \"write_initial\": "
           << (settings.write_initial ? "true" : "false")
           << ",\n"
              "      \"write_final\": "
           << (settings.write_final ? "true" : "false")
           << "\n"
              "    },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteDiagnostics(std::ostream& output, const MetadataDiagnostics& settings)
{
    output << "    \"diagnostics\": {\n"
              "      \"enabled\": "
           << (settings.enabled ? "true" : "false")
           << ",\n"
              "      \"every_steps\": "
           << settings.every_steps
           << ",\n"
              "      \"energy\": "
           << (settings.energy ? "true" : "false")
           << ",\n"
              "      \"momentum\": "
           << (settings.momentum ? "true" : "false")
           << ",\n"
              "      \"relative_error\": "
           << (settings.relative_error ? "true" : "false")
           << "\n"
              "    }\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteConfiguration(
    std::ostream& output, const MetadataRunConfiguration& configuration)
{
    output << "  \"configuration\": {\n";

    if (!WriteSimulation(output, configuration.simulation) ||
        !WriteGravity(output, configuration.gravity) || !WriteUnits(output, configuration.units) ||
        !WriteBarnesHut(output, configuration.barnes_hut)) {
        return false;
    }

    output << "    \"generation\": {\n"
              "      \"seed\": "
           << configuration.generation.seed
           << ",\n"
              "      \"deterministic\": "
           << (configuration.generation.deterministic ? "true" : "false")
           << "\n"
              "    },\n";

    return WriteOutput(output, configuration.output) &&
           WriteDiagnostics(output, configuration.diagnostics) && output << "  },\n";
}

[[nodiscard]] bool WriteCapabilities(std::ostream& output, const MetadataCapabilities& capabilities)
{
    output << "  \"capabilities\": {\n"
              "    \"implemented_solver_mask\": "
           << capabilities.implemented_solver_mask
           << ",\n"
              "    \"unsupported_solver_mask\": "
           << capabilities.unsupported_solver_mask
           << ",\n"
              "    \"deferred_feature_mask\": "
           << capabilities.deferred_feature_mask
           << ",\n"
              "    \"compiled_backend_mask\": "
           << capabilities.compiled_backend_mask
           << "\n"
              "  },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteDistribution(std::ostream& output, const MetadataRunInfo& info)
{
    output << "  \"distribution\": {\n"
              "    \"rank_count\": "
           << info.rank_count
           << ",\n"
              "    \"rank_index\": "
           << info.rank_index
           << "\n"
              "  },\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteCompletedOutputs(std::ostream& output,
    std::span<const std::uint64_t> completed_steps, MetadataOutputFormat format)
{
    output << "  \"completed_output_count\": " << completed_steps.size()
           << ",\n"
              "  \"completed_outputs\": [\n";

    for (std::size_t index = 0; index < completed_steps.size(); ++index) {
        const std::string file_name = StateFileName(completed_steps[index], format);

        if (file_name.empty()) {
            return false;
        }

        output << "    {\n      \"step\": " << completed_steps[index]
               << ",\n      \"path\": \"states/" << file_name << "\"\n    }"
               << (index + 1U == completed_steps.size() ? "\n" : ",\n");
    }

    output << "  ]\n";

    return static_cast<bool>(output);
}

} // namespace

bool MetadataManifest::WriteDocument(
    std::ostream& output, std::span<const std::uint64_t> completed_steps) const
{
    output.imbue(std::locale::classic());

    output << std::setprecision(17)
           << "{\n"
              "  \"schema_version\": 1,\n"
              "  \"product_version\": ";

    if (!WriteJsonString(output, info_.product_version)) {
        return false;
    }

    output << ",\n  \"plan_version\": ";

    if (!WriteJsonString(output, info_.plan_version)) {
        return false;
    }

    output << ",\n";

    return WriteConfiguration(output, info_.configuration) &&
           WriteCapabilities(output, info_.capabilities) && WriteDistribution(output, info_) &&
           WriteCompletedOutputs(output, completed_steps, info_.configuration.output.format) &&
           output << "}\n" && static_cast<bool>(output);
}

} // namespace blitzar_io
