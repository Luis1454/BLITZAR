/*
 * @file runtime/include/command/CommandTransport.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#include "protocol/ServerClient.hpp"
#include <cstdint>
#include <string>

namespace grav_cmd {
class CommandTransport {
public:
    virtual ~CommandTransport() = default;
    virtual bool connect(const std::string& host, std::uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual ServerClientResponse sendCommand(const std::string& cmd,
                                             const std::string& fieldsJson = "") = 0;
    virtual ServerClientResponse getStatus(ServerClientStatus& outStatus) = 0;
};

class ServerClientCommandTransport final : public CommandTransport {
public:
    explicit ServerClientCommandTransport(int timeoutMs = 150);
    bool connect(const std::string& host, std::uint16_t port) override;
    void disconnect() override;
    bool isConnected() const override;
    ServerClientResponse sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override;
    ServerClientResponse getStatus(ServerClientStatus& outStatus) override;

private:
    ServerClient _client;
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
