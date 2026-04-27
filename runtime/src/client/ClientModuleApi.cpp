// File: runtime/src/client/ClientModuleApi.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleApi.hpp"

namespace grav_module {
const std::uint32_t kClientModuleApiVersionV1 = 1u;
const char* kClientModuleEntryPoint = "gravity_client_module_v1";
const char* kClientModuleProductName = "BLITZAR";
const char* kClientModuleProductVersion = "0.0.0-dev";
} // namespace grav_module
