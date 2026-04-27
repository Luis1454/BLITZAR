// File: tests/int/ui/qt_workspace_overlay_controls.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/qt_test_utils.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include <QCheckBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QSpinBox>
#include <gtest/gtest.h>
#include <memory>
namespace grav_test_qt_workspace_overlay_controls {
/// Description: Defines the IdleRuntime data or behavior contract.
class IdleRuntime final : public grav_client::IClientRuntime {
public:
    /// Description: Executes the start operation.
    bool start() override
    {
        return false;
    }
    /// Description: Executes the stop operation.
    void stop() override
    {
    }
    /// Description: Executes the setPaused operation.
    void setPaused(bool) override
    {
    }
    /// Description: Executes the togglePaused operation.
    void togglePaused() override
    {
    }
    /// Description: Executes the stepOnce operation.
    void stepOnce() override
    {
    }
    /// Description: Executes the setParticleCount operation.
    void setParticleCount(std::uint32_t) override
    {
    }
    /// Description: Executes the setDt operation.
    void setDt(float) override
    {
    }
    /// Description: Executes the scaleDt operation.
    void scaleDt(float) override
    {
    }
    /// Description: Executes the requestReset operation.
    void requestReset() override
    {
    }
    /// Description: Executes the requestRecover operation.
    void requestRecover() override
    {
    }
    /// Description: Executes the setSolverMode operation.
    void setSolverMode(const std::string&) override
    {
    }
    /// Description: Executes the setIntegratorMode operation.
    void setIntegratorMode(const std::string&) override
    {
    }
    /// Description: Executes the setPerformanceProfile operation.
    void setPerformanceProfile(const std::string&) override
    {
    }
    /// Description: Executes the setOctreeParameters operation.
    void setOctreeParameters(float, float) override
    {
    }
    /// Description: Executes the setSphEnabled operation.
    void setSphEnabled(bool) override
    {
    }
    /// Description: Executes the setSphParameters operation.
    void setSphParameters(float, float, float, float) override
    {
    }
    /// Description: Executes the setSubstepPolicy operation.
    void setSubstepPolicy(float, std::uint32_t) override
    {
    }
    /// Description: Executes the setSnapshotPublishPeriodMs operation.
    void setSnapshotPublishPeriodMs(std::uint32_t) override
    {
    }
    /// Description: Executes the setInitialStateConfig operation.
    void setInitialStateConfig(const InitialStateConfig&) override
    {
    }
    /// Description: Executes the setEnergyMeasurementConfig operation.
    void setEnergyMeasurementConfig(std::uint32_t, std::uint32_t) override
    {
    }
    /// Description: Executes the setGpuTelemetryEnabled operation.
    void setGpuTelemetryEnabled(bool) override
    {
    }
    /// Description: Executes the setExportDefaults operation.
    void setExportDefaults(const std::string&, const std::string&) override
    {
    }
    /// Description: Executes the setInitialStateFile operation.
    void setInitialStateFile(const std::string&, const std::string&) override
    {
    }
    /// Description: Executes the requestExportSnapshot operation.
    void requestExportSnapshot(const std::string&, const std::string&) override
    {
    }
    /// Description: Executes the requestSaveCheckpoint operation.
    void requestSaveCheckpoint(const std::string&) override
    {
    }
    /// Description: Executes the requestLoadCheckpoint operation.
    void requestLoadCheckpoint(const std::string&) override
    {
    }
    /// Description: Executes the requestShutdown operation.
    void requestShutdown() override
    {
    }
    /// Description: Executes the setRemoteSnapshotCap operation.
    void setRemoteSnapshotCap(std::uint32_t) override
    {
    }
    /// Description: Executes the requestReconnect operation.
    void requestReconnect() override
    {
    }
    void configureRemoteConnector(const std::string&, std::uint16_t, bool,
                                  const std::string&) override
    {
    }
    /// Description: Executes the isRemoteMode operation.
    bool isRemoteMode() const override
    {
        return false;
    }
    /// Description: Executes the getCachedStats operation.
    SimulationStats getCachedStats() const override
    {
        return {};
    }
    /// Description: Executes the getStats operation.
    SimulationStats getStats() const override
    {
        return {};
    }
    /// Description: Executes the consumeLatestSnapshot operation.
    std::optional<grav_client::ConsumedSnapshot> consumeLatestSnapshot() override
    {
        return std::nullopt;
    }
    /// Description: Executes the tryConsumeSnapshot operation.
    bool tryConsumeSnapshot(std::vector<RenderParticle>&) override
    {
        return false;
    }
    /// Description: Executes the snapshotPipelineState operation.
    grav_client::SnapshotPipelineState snapshotPipelineState() const override
    {
        return {};
    }
    /// Description: Executes the linkStateLabel operation.
    std::string linkStateLabel() const override
    {
        return "disconnected";
    }
    /// Description: Executes the serverOwnerLabel operation.
    std::string serverOwnerLabel() const override
    {
        return "local";
    }
    /// Description: Executes the statsAgeMs operation.
    std::uint32_t statsAgeMs() const override
    {
        return 0u;
    }
    /// Description: Executes the snapshotAgeMs operation.
    std::uint32_t snapshotAgeMs() const override
    {
        return 0u;
    }
};
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_020_OctreeOverlayControlsUpdateWorkspaceView)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<IdleRuntime>();
    grav_qt::MainWindow window(SimulationConfig{}, "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    auto* multiViewWidget =
        dynamic_cast<grav_qt::MultiViewWidget*>(window.findChild<QWidget*>("multiViewWidget"));
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(multiViewWidget, nullptr);
    QCheckBox* overlayCheck = window.findChild<QCheckBox*>("octreeOverlayCheck");
    QSpinBox* depthSpin = window.findChild<QSpinBox*>("octreeOverlayDepthSpin");
    QSpinBox* opacitySpin = window.findChild<QSpinBox*>("octreeOverlayOpacitySpin");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(overlayCheck, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(depthSpin, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(opacitySpin, nullptr);
    overlayCheck->setChecked(true);
    depthSpin->setValue(4);
    opacitySpin->setValue(144);
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(multiViewWidget->octreeOverlayEnabled());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(multiViewWidget->octreeOverlayDepth(), 4);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(multiViewWidget->octreeOverlayOpacity(), 144);
}
} // namespace grav_test_qt_workspace_overlay_controls
