// File: runtime/src/client/ErrorBuffer.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ErrorBuffer.hpp"
#include <algorithm>

namespace grav_client {
/// Description: Describes the error buffer view operation contract.
ErrorBufferView::ErrorBufferView(char* buffer, std::size_t bufferSize) noexcept
    : m_buffer(buffer), m_bufferSize(bufferSize)
{
}

/// Description: Describes the buffer operation contract.
char* ErrorBufferView::buffer() const noexcept
{
    return m_buffer;
}

/// Description: Describes the buffer size operation contract.
std::size_t ErrorBufferView::bufferSize() const noexcept
{
    return m_bufferSize;
}

/// Description: Describes the has storage operation contract.
bool ErrorBufferView::hasStorage() const noexcept
{
    return m_buffer != nullptr && m_bufferSize != 0u;
}

/// Description: Describes the write operation contract.
void ErrorBufferView::write(std::string_view message) const noexcept
{
    writeErrorBuffer(m_buffer, m_bufferSize, message);
}

/// Description: Describes the write error buffer operation contract.
void writeErrorBuffer(char* buffer, std::size_t bufferSize, std::string_view message) noexcept
{
    if (buffer == nullptr || bufferSize == 0u) {
        return;
    }
    const std::size_t copySize = std::min(bufferSize - 1u, message.size());
    std::copy_n(message.data(), copySize, buffer);
    buffer[copySize] = '\0';
}
} // namespace grav_client
