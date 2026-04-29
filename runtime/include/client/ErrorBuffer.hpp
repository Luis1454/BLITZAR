/*
 * @file runtime/include/client/ErrorBuffer.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
#include <cstddef>
#include <string_view>

namespace bltzr_client {
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
} // namespace bltzr_client
#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_ERRORBUFFER_HPP_
