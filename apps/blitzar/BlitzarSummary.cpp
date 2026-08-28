#include "BlitzarSummary.hpp"

#include <locale>
#include <ostream>
#include <string>
#include <string_view>

namespace blitzar_cli {

namespace {

[[nodiscard]] bool WriteJsonString(std::ostream& output, std::string_view value) noexcept
{
    static constexpr char HexDigits[] = "0123456789abcdef";

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
                output << "\\u00" << HexDigits[character >> 4U] << HexDigits[character & 0x0fU];
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

[[nodiscard]] bool WriteOutputPath(std::ostream& output, const std::filesystem::path& path)
{
    const std::string portable_path = path.generic_string();

    return WriteJsonString(output, portable_path);
}

[[nodiscard]] std::string_view StatusName(blitzar_status status) noexcept
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

        return "singularity";

    case BLITZAR_STATUS_UNSUPPORTED:

        return "unsupported";

    default:

        return "unknown";
    }
}

[[nodiscard]] std::string_view SolverName(blitzar_solver_kind solver) noexcept
{
    switch (solver) {
    case BLITZAR_SOLVER_DIRECT:

        return "direct";

    case BLITZAR_SOLVER_BARNES_HUT:

        return "barnes-hut";

    case BLITZAR_SOLVER_FMM:

        return "fmm";

    case BLITZAR_SOLVER_PM:

        return "pm";

    case BLITZAR_SOLVER_TREEPM:

        return "treepm";

    default:

        return "unknown";
    }
}

[[nodiscard]] bool WriteJsonSummary(std::ostream& output, const BlitzarSummary& summary)
{
    output.imbue(std::locale::classic());

    output << "{\"schema_version\":" << BlitzarSummary::SchemaVersion
           << ",\"status\":" << summary.status << ",\"requested_steps\":" << summary.requested_steps
           << ",\"completed_steps\":" << summary.completed_steps
           << ",\"particle_count\":" << summary.particle_count << ",\"solver\":" << summary.solver
           << ",\"snapshot_count\":" << summary.snapshot_count
           << ",\"diagnostics_count\":" << summary.diagnostics_count << ",\"output_path\":";

    if (!WriteOutputPath(output, summary.output_path)) {
        return false;
    }

    output << "}\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteHumanSummary(std::ostream& output, const BlitzarSummary& summary)
{
    output.imbue(std::locale::classic());

    output << "BLITZAR result\n"
           << "  status:       " << StatusName(summary.status) << '\n'
           << "  solver:       " << SolverName(summary.solver) << '\n'
           << "  steps:        " << summary.completed_steps << '/' << summary.requested_steps
           << '\n'
           << "  particles:    " << summary.particle_count << '\n'
           << "  snapshots:    " << summary.snapshot_count << '\n'
           << "  diagnostics:  " << summary.diagnostics_count << '\n'
           << "  output:       ";

    if (summary.output_path.empty()) {
        output << "disabled";
    }
    else {
        output << summary.output_path.generic_string();
    }

    output << '\n';

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteJsonFailure(std::ostream& output, const BlitzarFailure& failure) noexcept
{
    const std::string_view message{blitzar_status_message(failure.status)};

    output.imbue(std::locale::classic());

    output << "{\"schema_version\":" << BlitzarSummary::SchemaVersion
           << ",\"status\":" << failure.status << ",\"phase\":";

    if (!WriteJsonString(output, failure.phase)) {
        return false;
    }

    output << ",\"exit_code\":" << static_cast<int>(failure.exit_code) << ",\"message\":";

    if (!WriteJsonString(output, message)) {
        return false;
    }

    output << "}\n";

    return static_cast<bool>(output);
}

[[nodiscard]] bool WriteHumanFailure(std::ostream& output, const BlitzarFailure& failure) noexcept
{
    const std::string_view message{blitzar_status_message(failure.status)};

    output.imbue(std::locale::classic());

    output << "BLITZAR error\n"
           << "  phase:        " << failure.phase << '\n'
           << "  status:       " << StatusName(failure.status) << '\n'
           << "  message:      " << message << '\n'
           << "  exit code:    " << static_cast<int>(failure.exit_code) << '\n';

    return static_cast<bool>(output);
}

} // namespace

bool WriteSummary(std::ostream& output, const BlitzarSummary& summary, BlitzarOutputFormat format)
{
    switch (format) {
    case BlitzarOutputFormat::Human:

        return WriteHumanSummary(output, summary);

    case BlitzarOutputFormat::Json:

        return WriteJsonSummary(output, summary);

    default:

        return false;
    }
}

bool WriteFailure(
    std::ostream& output, const BlitzarFailure& failure, BlitzarOutputFormat format) noexcept
{
    switch (format) {
    case BlitzarOutputFormat::Human:

        return WriteHumanFailure(output, failure);

    case BlitzarOutputFormat::Json:

        return WriteJsonFailure(output, failure);

    default:

        return false;
    }
}

} // namespace blitzar_cli
