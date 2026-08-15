/*
 * @file apps/desktop/src/Main.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Internal contract for the BLITZAR desktop entry point.
 */

#ifndef BLITZAR_APPS_DESKTOP_SRC_MAIN_HPP_
#define BLITZAR_APPS_DESKTOP_SRC_MAIN_HPP_

#include <string>

namespace bltzr_desktop {
int run(int argc, char** argv);
void showError(const std::string& message);
} // namespace bltzr_desktop

#endif // BLITZAR_APPS_DESKTOP_SRC_MAIN_HPP_
