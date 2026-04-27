// File: engine/src/platform/PlatformErrors.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/PlatformErrors.hpp"

namespace grav_platform_errors {
const std::string_view kInvalidLibraryHandleOrSymbol = "invalid library handle or symbol name";
const std::string_view kDynamicLibraryLoadFailed = "dynamic library load failed";
const std::string_view kMissingSymbolPrefix = "missing symbol: ";
const std::string_view kProcessLaunchFailed = "process launch failed";
const std::string_view kProcessTerminateFailed = "failed to terminate process";
const std::string_view kUnexpectedException = "unexpected exception";
const std::string_view kUnknownException = "unknown exception";
} // namespace grav_platform_errors
