// File: runtime/include/client/ClientModuleApi.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
#include <cstddef>
#include <cstdint>

namespace grav_module {
extern const std::uint32_t kClientModuleApiVersionV1;
extern const char* kClientModuleEntryPoint;
extern const char* kClientModuleProductName;
extern const char* kClientModuleProductVersion;

/// Description: Defines the ClientHostContextV1 data or behavior contract.
struct ClientHostContextV1 {
    const char* configPath;
};

/// Description: Describes the bool operation contract.
typedef bool (*ClientModuleCreateFn)(const ClientHostContextV1* context, void** outModuleState,
                                     char* errorBuffer, std::size_t errorBufferSize);
/// Description: Executes the void operation.
typedef void (*ClientModuleDestroyFn)(void* moduleState);
/// Description: Describes the bool operation contract.
typedef bool (*ClientModuleStartFn)(void* moduleState, char* errorBuffer,
                                    std::size_t errorBufferSize);
/// Description: Executes the void operation.
typedef void (*ClientModuleStopFn)(void* moduleState);
/// Description: Describes the bool operation contract.
typedef bool (*ClientModuleHandleCommandFn)(void* moduleState, const char* commandLine,
                                            bool* outKeepRunning, char* errorBuffer,
                                            std::size_t errorBufferSize);

/// Description: Defines the ClientModuleExportsV1 data or behavior contract.
struct ClientModuleExportsV1 {
    std::uint32_t apiVersion;
    const char* moduleName;
    ClientModuleCreateFn create;
    ClientModuleDestroyFn destroy;
    ClientModuleStartFn start;
    ClientModuleStopFn stop;
    ClientModuleHandleCommandFn handleCommand;
};

/// Description: Describes the client module exports v1* operation contract.
typedef const ClientModuleExportsV1* (*ClientModuleEntryPointFn)();
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
