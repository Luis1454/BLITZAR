/*
 * @file apps/desktop/src/PosixMain.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Portable console entry point for the BLITZAR desktop client.
 */

#include "Main.hpp"
#include <iostream>

namespace bltzr_desktop {
void showError(const std::string& message)
{
    std::cerr << "[blitzar-gui] " << message << '\n';
}
} // namespace bltzr_desktop

int main(int argc, char** argv)
{
    return bltzr_desktop::run(argc, argv);
}
