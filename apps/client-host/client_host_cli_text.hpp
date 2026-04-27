// File: apps/client-host/client_host_cli_text.hpp
// Purpose: Application entry point or host support for BLITZAR executables.

#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
#include <string>
#include <vector>
namespace grav_client_host {
/// Description: Defines the ClientHostCliText data or behavior contract.
class ClientHostCliText final {
public:
    /// Description: Executes the trim operation.
    static std::string trim(const std::string& input);
    /// Description: Executes the splitTokens operation.
    static std::vector<std::string> splitTokens(const std::string& line);
    /// Description: Executes the toLower operation.
    static std::string toLower(std::string value);
};
} // namespace grav_client_host
#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
