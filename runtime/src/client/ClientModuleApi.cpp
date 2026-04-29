/*
 * @file runtime/src/client/ClientModuleApi.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/ClientModuleApi.hpp"

namespace bltzr_module {
const std::uint32_t kClientModuleApiVersionV1 = 1u;
const char* kClientModuleEntryPoint = "BLITZAR_client_module_v1";
const char* kClientModuleProductName = "BLITZAR";
const char* kClientModuleProductVersion = "0.0.0-dev";
} // namespace bltzr_module
