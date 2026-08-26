#ifndef BLITZAR_TESTS_FIXTURES_ALLOCATION_MONITOR_HPP
#define BLITZAR_TESTS_FIXTURES_ALLOCATION_MONITOR_HPP

#include <cstddef>

namespace blitzar_tests {

void BeginAllocationCounting() noexcept;
[[nodiscard]] std::size_t EndAllocationCounting() noexcept;

} // namespace blitzar_tests

#endif
