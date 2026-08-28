#include "BlitzarPostProcess.hpp"

#include "BlitzarSummary.hpp"
#include "io/postprocess/PostProcess.hpp"

#include <blitzar/blitzar.h>
#include <filesystem>
#include <iostream>
#include <new>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace blitzar_cli {

namespace {

int PrintFailure(BlitzarStreams streams, std::string_view phase, blitzar_status status,
    BlitzarExitCode exit_code)
{
    const BlitzarFailure failure{status, phase, exit_code};

    (void)WriteFailure(streams.standard_error, failure);

    return static_cast<int>(exit_code);
}

[[nodiscard]] int WritePostProcessSummary(
    BlitzarStreams streams, const blitzar_io::PostProcessReport& report)
{
    const BlitzarSummary summary{BLITZAR_STATUS_OK, report.requested_steps, report.completed_steps,
        report.particle_count, report.solver, report.snapshot_count, report.diagnostics_count,
        report.output_path};

    return WriteSummary(streams.standard_output, summary)
               ? static_cast<int>(BlitzarExitCode::Success)
               : PrintFailure(
                     streams, "summary", BLITZAR_STATUS_INTERNAL_ERROR, BlitzarExitCode::Output);
}

} // namespace

int RunPostProcess(const std::filesystem::path& path)
{
    return RunPostProcess(path, {std::cout, std::cerr});
}

int RunPostProcess(const std::filesystem::path& path, BlitzarStreams streams)
{
    try {
        blitzar_io::PostProcess processor(path);
        blitzar_io::PostProcessReport report;
        const blitzar_status status = processor.Execute(report);

        if (status != BLITZAR_STATUS_OK) {
            return PrintFailure(streams, "post-process", status, BlitzarExitCode::Output);
        }

        return WritePostProcessSummary(streams, report);
    }
    catch (const std::length_error&) {
        return PrintFailure(
            streams, "post-process", BLITZAR_STATUS_INVALID_ARGUMENT, BlitzarExitCode::Output);
    }
    catch (const std::bad_alloc&) {
        return PrintFailure(
            streams, "post-process", BLITZAR_STATUS_ALLOCATION_FAILURE, BlitzarExitCode::Output);
    }
    catch (const std::filesystem::filesystem_error&) {
        return PrintFailure(
            streams, "post-process", BLITZAR_STATUS_INTERNAL_ERROR, BlitzarExitCode::Output);
    }
}

} // namespace blitzar_cli
