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

} // namespace

bool WriteSummary(std::ostream& output, const BlitzarSummary& summary)
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

bool WriteFailure(std::ostream& output, const BlitzarFailure& failure) noexcept
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

} // namespace blitzar_cli
