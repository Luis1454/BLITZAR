/*
 * @file runtime/include/client/module/Api.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
#include <cstddef>
#include <cstdint>

namespace bltzr_module {
extern const std::uint32_t kApiVersionV1;
extern const char* kEntryPoint;
extern const char* kProductName;
extern const char* kProductVersion;

struct HostContextV1 {
    const char* configPath = nullptr;
    const char* modulePath = nullptr;
};

typedef bool (*CreateFn)(const HostContextV1* context, void** outModuleState,
                                     char* errorBuffer, std::size_t errorBufferSize);
typedef void (*DestroyFn)(void* moduleState);
typedef bool (*StartFn)(void* moduleState, char* errorBuffer,
                                    std::size_t errorBufferSize);
typedef void (*StopFn)(void* moduleState);
typedef bool (*HandleCommandFn)(void* moduleState, const char* commandLine,
                                            bool* outKeepRunning, char* errorBuffer,
                                            std::size_t errorBufferSize);

struct ExportsV1 {
    std::uint32_t apiVersion;
    const char* moduleName;
    CreateFn create;
    DestroyFn destroy;
    StartFn start;
    StopFn stop;
    HandleCommandFn handleCommand;
};

typedef const ExportsV1* (*EntryPointFn)();
} // namespace bltzr_module
#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEAPI_HPP_
