#include "frontend/ErrorBuffer.hpp"

#include <algorithm>

namespace grav_frontend {

void writeErrorBuffer(char *buffer, std::size_t bufferSize, std::string_view message) noexcept
{
    if (buffer == nullptr || bufferSize == 0u) {
        return;
    }
    const std::size_t copySize = std::min(bufferSize - 1u, message.size());
    std::copy_n(message.data(), copySize, buffer);
    buffer[copySize] = '\0';
}

} // namespace grav_frontend
