// File: runtime/include/client/ErrorBuffer.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#include <cstddef>
#include <string_view>

namespace grav_client {
/// Description: Defines the ErrorBufferView data or behavior contract.
class ErrorBufferView final {
public:
    /// Description: Describes the error buffer view operation contract.
    ErrorBufferView(char* buffer, std::size_t bufferSize) noexcept;
    /// Description: Describes the buffer operation contract.
    [[nodiscard]] char* buffer() const noexcept;
    /// Description: Describes the buffer size operation contract.
    [[nodiscard]] std::size_t bufferSize() const noexcept;
    /// Description: Describes the has storage operation contract.
    [[nodiscard]] bool hasStorage() const noexcept;
    /// Description: Describes the write operation contract.
    void write(std::string_view message) const noexcept;

private:
    char* m_buffer;
    std::size_t m_bufferSize;
};

/// Description: Describes the write error buffer operation contract.
void writeErrorBuffer(char* buffer, std::size_t bufferSize, std::string_view message) noexcept;
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
