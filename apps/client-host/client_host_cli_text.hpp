/*
 * @file apps/client-host/client_host_cli_text.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
#define BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
#include <string>
#include <vector>

namespace bltzr_client_host {
class ClientHostCliText final {
public:
    static std::string trim(const std::string& input);
    static std::vector<std::string> splitTokens(const std::string& line);
    static std::string toLower(std::string value);
};
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
