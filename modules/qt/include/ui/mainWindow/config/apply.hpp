/*
 * Header for split config apply/capture helpers for MainWindow.
 */
#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_MAINWINDOW_CONFIG_APPLY_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_MAINWINDOW_CONFIG_APPLY_HPP_

#include "ui/MainWindow.hpp"

namespace bltzr_qt {

// Definitions for MainWindow methods moved out of MainWindowConfig.cpp
bool MainWindow_applyConfigToServer(MainWindow* self, bool requestReset);
void MainWindow_applyConfigToUi(MainWindow* self);
void MainWindow_captureUiIntoConfig(MainWindow* self);

} // namespace bltzr_qt

#endif
