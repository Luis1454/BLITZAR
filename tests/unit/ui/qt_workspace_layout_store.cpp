#include "ui/WorkspaceLayoutStore.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
namespace grav_test_qt_ui {
TEST(QtUiLogicTest, TST_UNT_UI_004_WorkspaceLayoutStoreRoundTripsNamedPreset)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("gravity_qt_workspace_" + std::to_string(stamp));
    const std::filesystem::path configPath = root / "simulation.ini";
    grav_qt::WorkspaceLayoutStore store(configPath.string());
    const std::string savedState = "dock-state-bytes";
    const std::string savedGeometry = "dock-geometry-bytes";
    ASSERT_TRUE(store.savePreset("Mission Deck 01", savedState, savedGeometry));
    const std::vector<std::string> presets = store.listPresets();
    ASSERT_EQ(presets.size(), 1u);
    EXPECT_EQ(presets.front(), "mission_deck_01");
    std::string loadedState;
    std::string loadedGeometry;
    ASSERT_TRUE(store.loadPreset("Mission Deck 01", loadedState, loadedGeometry));
    EXPECT_EQ(loadedState, savedState);
    EXPECT_EQ(loadedGeometry, savedGeometry);
    ASSERT_TRUE(store.deletePreset("Mission Deck 01"));
    EXPECT_TRUE(store.listPresets().empty());
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}
} // namespace grav_test_qt_ui
