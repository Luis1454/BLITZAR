#include "frontend/ErrorBuffer.hpp"

#include <algorithm>

namespace grav_frontend {

ErrorBufferView::ErrorBufferView(char *buffer, std::size_t bufferSize) noexcept
    : m_buffer(buffer), m_bufferSize(bufferSize)
{
}

char *ErrorBufferView::buffer() const noexcept
{
    return m_buffer;
}

std::size_t ErrorBufferView::bufferSize() const noexcept
{
    return m_bufferSize;
}

bool ErrorBufferView::hasStorage() const noexcept
{
    return m_buffer != nullptr && m_bufferSize != 0u;
}

void ErrorBufferView::write(std::string_view message) const noexcept
{
    writeErrorBuffer(m_buffer, m_bufferSize, message);
}

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
