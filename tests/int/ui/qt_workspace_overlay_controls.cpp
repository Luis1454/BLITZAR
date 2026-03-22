#include "tests/support/qt_test_utils.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QSpinBox>

#include <memory>

namespace grav_test_qt_workspace_overlay_controls {

class IdleRuntime final : public grav_client::IClientRuntime {
    public:
        bool start() override { return false; }
        void stop() override {}
        void setPaused(bool) override {}
        void togglePaused() override {}
        void stepOnce() override {}
        void setParticleCount(std::uint32_t) override {}
        void setDt(float) override {}
        void scaleDt(float) override {}
        void requestReset() override {}
        void requestRecover() override {}
        void setSolverMode(const std::string &) override {}
        void setIntegratorMode(const std::string &) override {}
        void setPerformanceProfile(const std::string &) override {}
        void setOctreeParameters(float, float) override {}
        void setSphEnabled(bool) override {}
        void setSphParameters(float, float, float, float) override {}
        void setSubstepPolicy(float, std::uint32_t) override {}
        void setSnapshotPublishPeriodMs(std::uint32_t) override {}
        void setInitialStateConfig(const InitialStateConfig &) override {}
        void setEnergyMeasurementConfig(std::uint32_t, std::uint32_t) override {}
        void setGpuTelemetryEnabled(bool) override {}
        void setExportDefaults(const std::string &, const std::string &) override {}
        void setInitialStateFile(const std::string &, const std::string &) override {}
        void requestExportSnapshot(const std::string &, const std::string &) override {}
        void requestShutdown() override {}
        void setRemoteSnapshotCap(std::uint32_t) override {}
        void requestReconnect() override {}
        void configureRemoteConnector(const std::string &, std::uint16_t, bool, const std::string &) override {}
        bool isRemoteMode() const override { return false; }
        SimulationStats getCachedStats() const override { return {}; }
        SimulationStats getStats() const override { return {}; }
        std::optional<grav_client::ConsumedSnapshot> consumeLatestSnapshot() override { return std::nullopt; }
        bool tryConsumeSnapshot(std::vector<RenderParticle> &) override { return false; }
        grav_client::SnapshotPipelineState snapshotPipelineState() const override { return {}; }
        std::string linkStateLabel() const override { return "disconnected"; }
        std::string serverOwnerLabel() const override { return "local"; }
        std::uint32_t statsAgeMs() const override { return 0u; }
        std::uint32_t snapshotAgeMs() const override { return 0u; }
};

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_020_OctreeOverlayControlsUpdateWorkspaceView)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<IdleRuntime>();
    grav_qt::MainWindow window(SimulationConfig{}, "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    auto *multiViewWidget = dynamic_cast<grav_qt::MultiViewWidget *>(window.findChild<QWidget *>("multiViewWidget"));
    ASSERT_NE(multiViewWidget, nullptr);

    QCheckBox *overlayCheck = window.findChild<QCheckBox *>("octreeOverlayCheck");
    QSpinBox *depthSpin = window.findChild<QSpinBox *>("octreeOverlayDepthSpin");
    QSpinBox *opacitySpin = window.findChild<QSpinBox *>("octreeOverlayOpacitySpin");
    ASSERT_NE(overlayCheck, nullptr);
    ASSERT_NE(depthSpin, nullptr);
    ASSERT_NE(opacitySpin, nullptr);

    overlayCheck->setChecked(true);
    depthSpin->setValue(4);
    opacitySpin->setValue(144);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    EXPECT_TRUE(multiViewWidget->octreeOverlayEnabled());
    EXPECT_EQ(multiViewWidget->octreeOverlayDepth(), 4);
    EXPECT_EQ(multiViewWidget->octreeOverlayOpacity(), 144);
}

} // namespace grav_test_qt_workspace_overlay_controls
