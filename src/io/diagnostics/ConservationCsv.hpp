#ifndef BLITZAR_IO_DIAGNOSTICS_CONSERVATION_CSV_HPP
#define BLITZAR_IO_DIAGNOSTICS_CONSERVATION_CSV_HPP

#include "physics/conservation/ConservationMetrics.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace blitzar_io {

inline constexpr std::string_view ConservationFileName = "conservation.csv";

struct ConservationSample final {
    std::uint64_t step{};
    blitzar_core::Scalar time{};
    std::uint64_t particle_count{};
    blitzar_physics::ConservationMetrics metrics{};
    bool write_energy{true};
    bool write_momentum{true};
    bool write_relative_error{true};
};

class ConservationCsv final {
public:
    explicit ConservationCsv(std::filesystem::path path);

    [[nodiscard]] blitzar_status Prepare() noexcept;
    [[nodiscard]] blitzar_status Append(ConservationSample sample) noexcept;
    [[nodiscard]] blitzar_status Close() noexcept;
    [[nodiscard]] std::size_t RecordCount() const noexcept;

private:
    std::filesystem::path path_;
    std::ofstream output_;
    blitzar_physics::ConservationMetrics reference_{};
    std::size_t record_count_{};
    bool has_reference_{};
    bool prepared_{};
};

} // namespace blitzar_io

#endif
