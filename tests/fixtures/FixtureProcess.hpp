#ifndef BLITZAR_TESTS_FIXTURES_FIXTURE_PROCESS_HPP
#define BLITZAR_TESTS_FIXTURES_FIXTURE_PROCESS_HPP

#include <filesystem>
#include <string_view>

namespace blitzar_test {

[[nodiscard]] bool RunProcess(const std::filesystem::path& executable, std::string_view mode,
    const std::filesystem::path& argument) noexcept;

} // namespace blitzar_test

#endif
