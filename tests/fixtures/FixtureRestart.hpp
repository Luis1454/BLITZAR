#ifndef BLITZAR_TESTS_FIXTURES_FIXTURE_RESTART_HPP
#define BLITZAR_TESTS_FIXTURES_FIXTURE_RESTART_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace blitzar_test {

[[nodiscard]] bool EnsureDirectory(const std::filesystem::path& path) noexcept;
[[nodiscard]] bool AcquireTestDirectory(
    std::filesystem::path& directory, std::string_view prefix) noexcept;
[[nodiscard]] bool RemoveTree(const std::filesystem::path& path) noexcept;
[[nodiscard]] bool SetMassAndRefreshChecksum(
    std::vector<std::uint8_t>& bytes, std::size_t particle_index, double mass);

} // namespace blitzar_test

#endif
