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
    /// Description: Describes the destroy  command transport operation contract.
    virtual ~CommandTransport() = default;
    /// Description: Describes the connect operation contract.
    virtual bool connect(const std::string& host, std::uint16_t port) = 0;
    /// Description: Describes the disconnect operation contract.
    virtual void disconnect() = 0;
    /// Description: Describes the is connected operation contract.
    virtual bool isConnected() const = 0;
    /// Description: Describes the send command operation contract.
    virtual ServerClientResponse sendCommand(const std::string& cmd,
                                             const std::string& fieldsJson = "") = 0;
    /// Description: Describes the get status operation contract.
    virtual ServerClientResponse getStatus(ServerClientStatus& outStatus) = 0;
};

/// Description: Defines the ServerClientCommandTransport data or behavior contract.
class ServerClientCommandTransport final : public CommandTransport {
public:
    /// Description: Describes the server client command transport operation contract.
    explicit ServerClientCommandTransport(int timeoutMs = 150);
    /// Description: Describes the connect operation contract.
    bool connect(const std::string& host, std::uint16_t port) override;
    /// Description: Describes the disconnect operation contract.
    void disconnect() override;
    /// Description: Describes the is connected operation contract.
    bool isConnected() const override;
    /// Description: Describes the send command operation contract.
    ServerClientResponse sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override;
    /// Description: Describes the get status operation contract.
    ServerClientResponse getStatus(ServerClientStatus& outStatus) override;

private:
    ServerClient _client;
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTRANSPORT_HPP_
