/*
 * @file runtime/src/client/module/Api.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/module/Api.hpp"

namespace bltzr_module {
const std::uint32_t kApiVersionV1 = 1u;
const char* kEntryPoint = "BLITZAR_client_module_v1";
const char* kProductName = "BLITZAR";
const char* kProductVersion = "0.0.0-dev";
} // namespace bltzr_module
