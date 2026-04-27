// File: runtime/include/client/ErrorBuffer.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#include <cstddef>
#include <string_view>
namespace grav_client {
class ErrorBufferView final {
public:
    ErrorBufferView(char* buffer, std::size_t bufferSize) noexcept;
    [[nodiscard]] char* buffer() const noexcept;
    [[nodiscard]] std::size_t bufferSize() const noexcept;
    [[nodiscard]] bool hasStorage() const noexcept;
    void write(std::string_view message) const noexcept;

private:
    char* m_buffer;
    std::size_t m_bufferSize;
};
void writeErrorBuffer(char* buffer, std::size_t bufferSize, std::string_view message) noexcept;
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
