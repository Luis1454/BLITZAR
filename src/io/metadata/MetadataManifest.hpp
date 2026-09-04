#ifndef BLITZAR_IO_METADATA_METADATA_MANIFEST_HPP
#define BLITZAR_IO_METADATA_METADATA_MANIFEST_HPP

#include "core/CoreSnapshot.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

namespace blitzar_io {

inline constexpr std::uint64_t MetadataMaxStepCount = 100000;
inline constexpr std::uint64_t MetadataMaxStateStep = 99999999;
inline constexpr std::uint32_t MetadataMaxRankIndex = 99999999;

struct MetadataSimulation final {
    std::uint64_t particle_count{};
    std::uint64_t requested_steps{};
    double timestep{};
    blitzar_solver_kind solver{BLITZAR_SOLVER_DIRECT};
    blitzar_integrator_kind integrator{BLITZAR_INTEGRATOR_LEAPFROG_KDK};
};

struct MetadataGravity final {
    double gravitational_constant{};
    double softening{};
};

struct MetadataUnits final {
    double length_scale{1.0};
    double mass_scale{1.0};
    double time_scale{1.0};
};

struct MetadataBarnesHut final {
    double opening_angle{0.5};
    std::uint64_t max_particles{};
    std::uint64_t max_cells{};
    std::uint64_t leaf_capacity{8};
    std::uint64_t max_depth{32};
};

struct MetadataGeneration final {
    std::uint64_t seed{};
    bool deterministic{};
};

enum class MetadataOutputFormat : std::uint8_t {
    Binary,
    Hdf5,
};

struct MetadataOutput final {
    bool enabled{};
    std::uint64_t every_steps{};
    bool write_initial{};
    bool write_final{};
    MetadataOutputFormat format{MetadataOutputFormat::Binary};
};

struct MetadataDiagnostics final {
    bool enabled{};
    std::uint64_t every_steps{};
    bool energy{};
    bool momentum{};
    bool relative_error{};
};

struct MetadataRunConfiguration final {
    MetadataSimulation simulation{};
    MetadataGravity gravity{};
    MetadataUnits units{};
    MetadataBarnesHut barnes_hut{};
    MetadataGeneration generation{};
    MetadataOutput output{};
    MetadataDiagnostics diagnostics{};
};

struct MetadataCapabilities final {
    blitzar_solver_mask implemented_solver_mask{};
    blitzar_solver_mask unsupported_solver_mask{};
    blitzar_feature_mask deferred_feature_mask{};
    blitzar_compiled_backend_mask compiled_backend_mask{};
};

struct MetadataRunInfo final {
    std::string product_version;
    std::string plan_version;
    MetadataRunConfiguration configuration{};
    MetadataCapabilities capabilities{};
    std::uint32_t rank_count{1};
    std::uint32_t rank_index{};

    [[nodiscard]] blitzar_status Validate() const noexcept;
};

[[nodiscard]] std::string StateFileName(std::uint64_t step);
[[nodiscard]] std::string StateFileName(std::uint64_t step, MetadataOutputFormat format);
[[nodiscard]] std::string StateShardFileName(
    std::uint64_t step, std::uint32_t rank_index, MetadataOutputFormat format);
[[nodiscard]] std::string_view MetadataOutputFormatName(MetadataOutputFormat format) noexcept;
[[nodiscard]] bool ParseMetadataOutputFormat(
    std::string_view text, MetadataOutputFormat& format) noexcept;

class MetadataManifest final {
public:
    explicit MetadataManifest(MetadataRunInfo info);

    [[nodiscard]] const MetadataRunInfo& Info() const noexcept;
    [[nodiscard]] blitzar_status WriteAtomic(
        const std::filesystem::path& path, std::span<const std::uint64_t> completed_steps) const;

private:
    [[nodiscard]] blitzar_status WriteFile(
        const std::filesystem::path& path, std::span<const std::uint64_t> completed_steps) const;

    [[nodiscard]] bool WriteDocument(
        std::ostream& output, std::span<const std::uint64_t> completed_steps) const;

    MetadataRunInfo info_;
};

} // namespace blitzar_io

#endif
