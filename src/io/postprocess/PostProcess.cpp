#include "io/postprocess/PostProcess.hpp"

#include "io/diagnostics/ConservationCsv.hpp"
#include "io/postprocess/PostProcessOutput.hpp"
#include "io/postprocess/PostProcessState.hpp"

#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_io {

namespace {

[[nodiscard]] blitzar_status ExecutePostProcess(
    const std::filesystem::path& run_directory, PostProcessReport& report)
{
    PostProcessInput input;
    const blitzar_status input_status = ReadPostProcessInput(run_directory, input);

    if (input_status != BLITZAR_STATUS_OK) {
        return input_status;
    }

    PostProcessOutput output(run_directory / "postProcessing");
    const blitzar_status prepare_status = output.Prepare();

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    ConservationCsv csv(output.TemporaryPath());
    const blitzar_status csv_status = csv.Prepare();

    if (csv_status != BLITZAR_STATUS_OK) {
        return csv_status;
    }

    const blitzar_status process_status = ProcessSnapshots(input, csv);

    if (process_status != BLITZAR_STATUS_OK) {
        return process_status;
    }

    const blitzar_status close_status = csv.Close();

    if (close_status != BLITZAR_STATUS_OK) {
        return close_status;
    }

    const blitzar_status commit_status = output.Commit();

    if (commit_status != BLITZAR_STATUS_OK) {
        return commit_status;
    }

    report.requested_steps = input.info.configuration.simulation.requested_steps;
    report.completed_steps = input.completed_steps.back();
    report.particle_count = input.info.configuration.simulation.particle_count;
    report.solver = input.info.configuration.simulation.solver;
    report.snapshot_count = input.completed_steps.size();
    report.diagnostics_count = csv.RecordCount();
    report.output_path = output.FinalPath();

    return BLITZAR_STATUS_OK;
}

} // namespace

PostProcess::PostProcess(std::filesystem::path run_directory)
    : run_directory_(std::move(run_directory).lexically_normal())
{
}

blitzar_status PostProcess::Execute(PostProcessReport& report) noexcept
{
    try {
        return ExecutePostProcess(run_directory_, report);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::filesystem::filesystem_error&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace blitzar_io
