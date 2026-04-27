// File: runtime/include/command/CommandTransport.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
#include "protocol/ServerClient.hpp"
#include <cstdint>
#include <string>
namespace grav_cmd {
/// Description: Defines the CommandTransport data or behavior contract.
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
/// Description: Defines the ServerClientCommandTransport data or behavior contract.
class ServerClientCommandTransport final : public CommandTransport {
public:
    /// Description: Executes the ServerClientCommandTransport operation.
    explicit ServerClientCommandTransport(int timeoutMs = 150);
    /// Description: Executes the connect operation.
    bool connect(const std::string& host, std::uint16_t port) override;
    /// Description: Executes the disconnect operation.
    void disconnect() override;
    /// Description: Executes the isConnected operation.
    bool isConnected() const override;
    ServerClientResponse sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override;
    /// Description: Executes the getStatus operation.
    ServerClientResponse getStatus(ServerClientStatus& outStatus) override;

private:
    ServerClient _client;
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
