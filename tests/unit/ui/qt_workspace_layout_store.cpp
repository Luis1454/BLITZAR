// File: tests/unit/ui/qt_workspace_layout_store.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "ui/WorkspaceLayoutStore.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
namespace grav_test_qt_ui {
/// Description: Executes the TEST operation.
TEST(QtUiLogicTest, TST_UNT_UI_004_WorkspaceLayoutStoreRoundTripsNamedPreset)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path root =
        /// Description: Executes the temp_directory_path operation.
        std::filesystem::temp_directory_path() / ("gravity_qt_workspace_" + std::to_string(stamp));
    const std::filesystem::path configPath = root / "simulation.ini";
    /// Description: Executes the store operation.
    grav_qt::WorkspaceLayoutStore store(configPath.string());
    const std::string savedState = "dock-state-bytes";
    const std::string savedGeometry = "dock-geometry-bytes";
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(store.savePreset("Mission Deck 01", savedState, savedGeometry));
    const std::vector<std::string> presets = store.listPresets();
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(presets.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(presets.front(), "mission_deck_01");
    std::string loadedState;
    std::string loadedGeometry;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(store.loadPreset("Mission Deck 01", loadedState, loadedGeometry));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loadedState, savedState);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loadedGeometry, savedGeometry);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(store.deletePreset("Mission Deck 01"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(store.listPresets().empty());
    std::error_code ec;
    /// Description: Executes the remove_all operation.
    std::filesystem::remove_all(root, ec);
}
} // namespace grav_test_qt_ui
