/*
 * @file apps/client-host/include/CliText.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_CLIENT_HOST_CLITEXT_HPP_
#define BLITZAR_APPS_CLIENT_HOST_CLITEXT_HPP_
#include <string>
#include <vector>

namespace bltzr_client_host {
    class ClientHostCliTextLocal final {
    public:
        static std::string trim(const std::string& input);
        static std::vector<std::string> splitTokens(const std::string& line);
        static std::string toLower(std::string value);
    };

    class ClientHostCliText final {
    public:
        static std::string trim(const std::string& input);
        static std::vector<std::string> splitTokens(const std::string& line);
        static std::string toLower(std::string value);
    };
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_CLIENT_HOST_CLITEXT_HPP_
