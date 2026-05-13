/*
 * @file runtime/include/command/transport/Transport.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#include "protocol/client/Client.hpp"
#include <cstdint>
#include <string>

namespace bltzr_cmd {
class Transport {
public:
    virtual ~Transport() = default;
    virtual bool connect(const std::string& host, std::uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bltzr_protocol::Response sendCommand(const std::string& cmd,
                                             const std::string& fieldsJson = "") = 0;
    virtual bltzr_protocol::Response getStatus(bltzr_protocol::ClientStatus& outStatus) = 0;
};

class ServerTransport final : public Transport {
public:
    explicit ServerTransport(int timeoutMs = 150);
    bool connect(const std::string& host, std::uint16_t port) override;
    void disconnect() override;
    bool isConnected() const override;
    bltzr_protocol::Response sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override;
    bltzr_protocol::Response getStatus(bltzr_protocol::ClientStatus& outStatus) override;

private:
    bltzr_protocol::Client _client;
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
