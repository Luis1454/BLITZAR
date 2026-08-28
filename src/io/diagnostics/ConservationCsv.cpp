#include "io/diagnostics/ConservationCsv.hpp"

#include <cmath>
#include <iomanip>
#include <locale>
#include <ostream>
#include <utility>

namespace blitzar_io {

namespace {

constexpr std::string_view Header =
    "step,time,particle_count,kinetic_energy,potential_energy,total_energy,momentum_x,momentum_y,"
    "momentum_z,relative_energy_error,relative_momentum_error\n";

[[nodiscard]] bool IsValidSample(const ConservationSample& sample) noexcept
{
    return sample.particle_count > 0 && std::isfinite(sample.time) && sample.metrics.IsFinite();
}

[[nodiscard]] bool RelativeError(blitzar_core::Scalar current, blitzar_core::Scalar reference,
    blitzar_core::Scalar& result) noexcept
{
    const blitzar_core::Scalar difference = std::abs(current - reference);
    const blitzar_core::Scalar baseline = std::abs(reference);

    if (!std::isfinite(difference) || !std::isfinite(baseline)) {
        return false;
    }

    result = baseline == 0.0 ? difference : difference / baseline;

    return std::isfinite(result);
}

[[nodiscard]] bool MomentumRelativeError(const blitzar_core::Vector3& current,
    const blitzar_core::Vector3& reference, blitzar_core::Scalar& result) noexcept
{
    const blitzar_core::Scalar current_norm =
        std::hypot(std::hypot(current.x, current.y), current.z);

    const blitzar_core::Scalar reference_norm =
        std::hypot(std::hypot(reference.x, reference.y), reference.z);

    const blitzar_core::Scalar difference = std::hypot(
        std::hypot(current.x - reference.x, current.y - reference.y), current.z - reference.z);

    if (!std::isfinite(current_norm) || !std::isfinite(reference_norm) ||
        !std::isfinite(difference)) {
        return false;
    }

    result = reference_norm == 0.0 ? difference : difference / reference_norm;

    return std::isfinite(result);
}

void WriteScalar(std::ostream& output, bool enabled, blitzar_core::Scalar value)
{
    if (enabled) {
        output << value;
    }
    else {
        output << "nan";
    }
}

[[nodiscard]] bool WriteRow(const ConservationSample& sample,
    blitzar_core::Scalar relative_energy_error, blitzar_core::Scalar relative_momentum_error,
    std::ostream& output)
{
    output << sample.step << ',' << std::setprecision(17) << sample.time << ','
           << sample.particle_count << ',';

    WriteScalar(output, sample.write_energy, sample.metrics.kinetic_energy);

    output << ',';

    WriteScalar(output, sample.write_energy, sample.metrics.potential_energy);

    output << ',';

    WriteScalar(output, sample.write_energy, sample.metrics.total_energy);

    output << ',';

    WriteScalar(output, sample.write_momentum, sample.metrics.momentum.x);

    output << ',';

    WriteScalar(output, sample.write_momentum, sample.metrics.momentum.y);

    output << ',';

    WriteScalar(output, sample.write_momentum, sample.metrics.momentum.z);

    output << ',';

    WriteScalar(output, sample.write_relative_error, relative_energy_error);

    output << ',';

    WriteScalar(output, sample.write_relative_error, relative_momentum_error);

    output << '\n';

    return static_cast<bool>(output);
}

} // namespace

ConservationCsv::ConservationCsv(std::filesystem::path path) : path_(std::move(path)) {}

blitzar_status ConservationCsv::Prepare() noexcept
{
    if (prepared_ || path_.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    output_.open(path_, std::ios::binary | std::ios::trunc);

    if (!output_.is_open()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    output_.imbue(std::locale::classic());

    output_ << Header;

    record_count_ = 0U;
    reference_ = {};
    has_reference_ = false;

    if (!output_) {
        output_.close();

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    prepared_ = true;

    return BLITZAR_STATUS_OK;
}

blitzar_status ConservationCsv::Append(ConservationSample sample) noexcept
{
    if (!prepared_ || !IsValidSample(sample)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_core::Scalar relative_energy_error{};
    blitzar_core::Scalar relative_momentum_error{};

    if (has_reference_) {
        if (!RelativeError(
                sample.metrics.total_energy, reference_.total_energy, relative_energy_error) ||
            !MomentumRelativeError(
                sample.metrics.momentum, reference_.momentum, relative_momentum_error)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    if (!WriteRow(sample, relative_energy_error, relative_momentum_error, output_)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    if (!has_reference_) {
        reference_ = sample.metrics;
        has_reference_ = true;
    }

    ++record_count_;

    output_.flush();

    return output_ ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status ConservationCsv::Close() noexcept
{
    if (!prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    output_.flush();

    output_.close();

    prepared_ = false;

    return output_ ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

std::size_t ConservationCsv::RecordCount() const noexcept
{
    return record_count_;
}

} // namespace blitzar_io
