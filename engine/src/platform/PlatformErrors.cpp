/*
 * @file engine/src/platform/PlatformErrors.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction implementation for portable runtime services.
 */

#include "platform/PlatformErrors.hpp"

namespace bltzr_platform_errors {
const std::string_view kInvalidLibraryHandleOrSymbol = "invalid library handle or symbol name";
const std::string_view kDynamicLibraryLoadFailed = "dynamic library load failed";
const std::string_view kMissingSymbolPrefix = "missing symbol: ";
const std::string_view kProcessLaunchFailed = "process launch failed";
const std::string_view kProcessTerminateFailed = "failed to terminate process";
const std::string_view kUnexpectedException = "unexpected exception";
const std::string_view kUnknownException = "unknown exception";
} // namespace bltzr_platform_errors
