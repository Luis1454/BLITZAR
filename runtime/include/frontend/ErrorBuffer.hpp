#ifndef GRAVITY_SIM_ERRORBUFFER_HPP
#define GRAVITY_SIM_ERRORBUFFER_HPP

#include <cstddef>
#include <string_view>

namespace grav_frontend {

void writeErrorBuffer(char *buffer, std::size_t bufferSize, std::string_view message) noexcept;

} // namespace grav_frontend

#endif // GRAVITY_SIM_ERRORBUFFER_HPP
