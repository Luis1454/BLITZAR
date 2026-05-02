#ifndef BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_HELPERS_HPP_
#define BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_HELPERS_HPP_

#include "platform/SocketPlatform.hpp"
#include <string>
#include <string_view>

std::string daemonTrim(const std::string& input);
bltzr_socket::ConstBytes daemonAsBytes(std::string_view text);
bool daemonSendAll(bltzr_socket::Handle socketHandle, bltzr_socket::ConstBytes bytes);
std::string daemonServerError(std::string_view operation, std::string_view detail);

#endif // BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_HELPERS_HPP_
