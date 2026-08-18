/*
 * @file modules/qt/window/control/GuiControls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface control connection coordinator.
 */

#include "window/core/GuiWindow.hpp"

namespace bltzr_qt {
void Window::connectControls()
{
    connectRunControls();
    connectSceneControls();
    connectPhysicsControls();
    connectRenderControls();
}
} // namespace bltzr_qt
