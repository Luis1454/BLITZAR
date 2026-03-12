#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_

#include <string>
#include <vector>

namespace grav_module_host {

class ModuleHostCliText final {
public:
    static std::string trim(const std::string &input);
    static std::vector<std::string> splitTokens(const std::string &line);
    static std::string toLower(std::string value);
};

} // namespace grav_module_host

#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_TEXT_HPP_
