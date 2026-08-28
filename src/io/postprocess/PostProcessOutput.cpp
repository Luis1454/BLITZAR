#include "io/postprocess/PostProcessOutput.hpp"

#include "io/diagnostics/ConservationCsv.hpp"

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace blitzar_io {

namespace {

[[nodiscard]] bool IsEmptyDirectory(const std::filesystem::path& path, std::error_code& error)
{
    const std::filesystem::directory_iterator first(path, error);

    if (error) {
        return false;
    }

    return first == std::filesystem::directory_iterator{};
}

[[nodiscard]] blitzar_status PrepareDirectory(
    const std::filesystem::path& path, bool& created) noexcept
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code status_error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, status_error);

    if (status_error &&
        status_error != std::make_error_code(std::errc::no_such_file_or_directory)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    status_error.clear();

    if (!std::filesystem::exists(status)) {
        std::filesystem::create_directory(path, status_error);

        if (status_error) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        created = true;

        return BLITZAR_STATUS_OK;
    }

    if (!std::filesystem::is_directory(status) || !IsEmptyDirectory(path, status_error)) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace

PostProcessOutput::PostProcessOutput(std::filesystem::path directory)
    : directory_(std::move(directory)),
      temporary_path_(directory_ / (std::string(ConservationFileName) + ".tmp")),
      final_path_(directory_ / ConservationFileName)
{
}

PostProcessOutput::~PostProcessOutput()
{
    Abort();
}

blitzar_status PostProcessOutput::Prepare() noexcept
{
    if (prepared_ || committed_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status status = PrepareDirectory(directory_, directory_created_);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    prepared_ = true;

    return BLITZAR_STATUS_OK;
}

blitzar_status PostProcessOutput::Commit() noexcept
{
    if (!prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code error;

    std::filesystem::rename(temporary_path_, final_path_, error);

    if (error) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    prepared_ = false;
    committed_ = true;

    return BLITZAR_STATUS_OK;
}

void PostProcessOutput::Abort() noexcept
{
    if (committed_) {
        return;
    }

    std::error_code error;

    std::filesystem::remove(temporary_path_, error);

    if (directory_created_) {
        error.clear();
        std::filesystem::remove(directory_, error);
    }

    prepared_ = false;
}

const std::filesystem::path& PostProcessOutput::TemporaryPath() const noexcept
{
    return temporary_path_;
}

const std::filesystem::path& PostProcessOutput::FinalPath() const noexcept
{
    return final_path_;
}

} // namespace blitzar_io
