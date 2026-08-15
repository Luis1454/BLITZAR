/*
 * @file modules/qt/src/window/config/ConfigurationEditor.hpp
 * @brief Structured editor for the loaded simulation configuration.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WINDOW_CONFIG_CONFIGURATIONEDITOR_HPP_
#define BLITZAR_MODULES_QT_SRC_WINDOW_CONFIG_CONFIGURATIONEDITOR_HPP_

#include "config/core/Config.hpp"
#include <QDialog>
#include <QGroupBox>
#include <QHash>

class QWidget;
class QComboBox;
class QFormLayout;
class QString;
class QVBoxLayout;

namespace bltzr_qt {
class ConfigurationEditor final : public QDialog {
public:
    explicit ConfigurationEditor(const SimulationConfig& config, QWidget* parent = nullptr);

    SimulationConfig configuration() const;
    void reload(const SimulationConfig& config);

private:
    void buildTiles(const SimulationConfig& config);
    QFormLayout* addTile(QVBoxLayout* stack, const QString& section, const QString& title,
                         const QString& description);
    void readValues(SimulationConfig& config) const;
    void addSimulationTile(const SimulationConfig& config);
    void addSceneTile(const SimulationConfig& config);
    void addSceneTransformTile(const SimulationConfig& config);
    void addBodyTiles(const SimulationConfig& config);
    void addPhysicsTiles(const SimulationConfig& config);
    void addRuntimeTiles(const SimulationConfig& config);

    SimulationConfig _configuration;
    QHash<QString, QWidget*> _fields;
    QWidget* _content = nullptr;
    QVBoxLayout* _stack = nullptr;
};
} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_WINDOW_CONFIG_CONFIGURATIONEDITOR_HPP_
