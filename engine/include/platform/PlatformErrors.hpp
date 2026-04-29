/*
 * @file engine/include/platform/PlatformErrors.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
#define BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
#include <string_view>

namespace bltzr_platform_errors {
extern const std::string_view kInvalidLibraryHandleOrSymbol;
extern const std::string_view kDynamicLibraryLoadFailed;
extern const std::string_view kMissingSymbolPrefix;
extern const std::string_view kProcessLaunchFailed;
extern const std::string_view kProcessTerminateFailed;
extern const std::string_view kUnexpectedException;
extern const std::string_view kUnknownException;
} // namespace bltzr_platform_errors
#endif // BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
