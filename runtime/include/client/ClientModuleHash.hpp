// File: runtime/include/client/ClientModuleHash.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
#include <string>
#include <string_view>
namespace grav_module {
/// Description: Defines the ClientModuleHash data or behavior contract.
class ClientModuleHash final {
public:
    static bool computeFileSha256Hex(std::string_view filePath, std::string& outHexDigest,
                                     std::string& outError);
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHASH_HPP_
