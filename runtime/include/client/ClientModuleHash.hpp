#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
#include <string>
#include <string_view>
namespace grav_module {
class ClientModuleHash final {
public:
    static bool computeFileSha256Hex(std::string_view filePath, std::string& outHexDigest,
                                     std::string& outError);
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
