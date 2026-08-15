/*
 * @file apps/desktop/src/WindowsMain.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Console-free Windows entry point for the BLITZAR desktop client.
 */

#include "Main.hpp"
#include <windows.h>

namespace bltzr_desktop {
void showError(const std::string& message)
{
    MessageBoxA(nullptr, message.c_str(), "BLITZAR GUI", MB_OK | MB_ICONERROR);
}
} // namespace bltzr_desktop

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return bltzr_desktop::run(__argc, __argv);
}
