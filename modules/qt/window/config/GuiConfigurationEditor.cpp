/*
 * @file modules/qt/window/config/GuiConfigurationEditor.cpp
 * @brief Structured tile editor for all persisted simulation settings.
 */

#include "window/config/GuiConfigurationEditor.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>
#include <algorithm>
#include <climits>
#include <limits>
#include <memory>

namespace bltzr_qt {
QPointer<QWidget> addInt(QPointer<QFormLayout> form, QHash<QString, QPointer<QWidget>>& fields,
                         const QString& key, int value, int minimum = 0, int maximum = INT_MAX)
{
    QPointer<QSpinBox> editor = new QSpinBox(form->parentWidget());
    editor->setObjectName(key);
    editor->setRange(minimum, maximum);
    editor->setValue(value);
    fields.insert(key, editor);
    form->addRow(key, editor);
    return editor;
}

QPointer<QWidget> addFloat(QPointer<QFormLayout> form, QHash<QString, QPointer<QWidget>>& fields,
                           const QString& key, double value, double minimum = -1.0e12,
                           double maximum = 1.0e12, int decimals = 6)
{
    QPointer<QDoubleSpinBox> editor = new QDoubleSpinBox(form->parentWidget());
    editor->setObjectName(key);
    editor->setDecimals(decimals);
    editor->setRange(minimum, maximum);
    editor->setSingleStep(0.01);
    editor->setValue(value);
    fields.insert(key, editor);
    form->addRow(key, editor);
    return editor;
}

QPointer<QWidget> addBool(QPointer<QFormLayout> form, QHash<QString, QPointer<QWidget>>& fields,
                          const QString& key, bool value)
{
    QPointer<QCheckBox> editor = new QCheckBox(form->parentWidget());
    editor->setObjectName(key);
    editor->setChecked(value);
    fields.insert(key, editor);
    form->addRow(key, editor);
    return editor;
}

QPointer<QWidget> addText(QPointer<QFormLayout> form, QHash<QString, QPointer<QWidget>>& fields,
                          const QString& key, const std::string& value)
{
    QPointer<QLineEdit> editor = new QLineEdit(QString::fromStdString(value), form->parentWidget());
    editor->setObjectName(key);
    fields.insert(key, editor);
    form->addRow(key, editor);
    return editor;
}

QPointer<QWidget> addCombo(QPointer<QFormLayout> form, QHash<QString, QPointer<QWidget>>& fields,
                           const QString& key, const std::string& value, const QStringList& choices)
{
    QPointer<QComboBox> editor = new QComboBox(form->parentWidget());
    editor->setObjectName(key);
    editor->addItems(choices);
    const int index = editor->findText(QString::fromStdString(value));
    editor->setCurrentIndex(index < 0 ? 0 : index);
    fields.insert(key, editor);
    form->addRow(key, editor);
    return editor;
}

QPointer<QWidget> field(const QHash<QString, QPointer<QWidget>>& fields, const QString& key)
{
    return fields.value(key);
}

bool fieldIsActive(QPointer<QWidget> editor)
{
    if (editor == nullptr)
        return false;
    for (QPointer<QWidget> parent = editor; parent != nullptr; parent = parent->parentWidget()) {
        if (parent->property("configSection").isValid())
            return parent->isVisible();
    }
    return true;
}

void readInt(const QHash<QString, QPointer<QWidget>>& fields, const QString& key,
             std::uint32_t& target)
{
    if (const QPointer<QWidget> widget = field(fields, key); fieldIsActive(widget)) {
        const QPointer<QSpinBox> editor = qobject_cast<QSpinBox*>(widget);
        if (editor != nullptr)
            target = static_cast<std::uint32_t>(editor->value());
    }
}

void readInt(const QHash<QString, QPointer<QWidget>>& fields, const QString& key, int& target)
{
    if (const QPointer<QWidget> widget = field(fields, key); fieldIsActive(widget)) {
        const QPointer<QSpinBox> editor = qobject_cast<QSpinBox*>(widget);
        if (editor != nullptr)
            target = editor->value();
    }
}

void readFloat(const QHash<QString, QPointer<QWidget>>& fields, const QString& key, float& target)
{
    if (const QPointer<QWidget> widget = field(fields, key); fieldIsActive(widget)) {
        const QPointer<QDoubleSpinBox> editor = qobject_cast<QDoubleSpinBox*>(widget);
        if (editor != nullptr)
            target = static_cast<float>(editor->value());
    }
}

void readBool(const QHash<QString, QPointer<QWidget>>& fields, const QString& key, bool& target)
{
    if (const QPointer<QWidget> widget = field(fields, key); fieldIsActive(widget)) {
        const QPointer<QCheckBox> editor = qobject_cast<QCheckBox*>(widget);
        if (editor != nullptr)
            target = editor->isChecked();
    }
}

void readString(const QHash<QString, QPointer<QWidget>>& fields, const QString& key,
                std::string& target)
{
    const QPointer<QWidget> widget = field(fields, key);
    if (!fieldIsActive(widget))
        return;
    const QPointer<QLineEdit> editor = qobject_cast<QLineEdit*>(widget);
    const QPointer<QComboBox> combo = qobject_cast<QComboBox*>(widget);
    if (editor != nullptr)
        target = editor->text().toStdString();
    else if (combo != nullptr)
        target = combo->currentText().toStdString();
}

ConfigurationEditor::ConfigurationEditor(const SimulationConfig& config, QPointer<QWidget> parent)
    : QDialog(parent), _configuration(config)
{
    setWindowTitle("Configuration");
    setObjectName("configurationEditorDialog");
    resize(760, 820);
    QPointer<QVBoxLayout> root = new QVBoxLayout(this);
    QPointer<QLabel> intro = new QLabel(
        "Advanced input/output settings only. Solver, physics, rendering, "
        "scene objects and properties are managed in their dedicated menus.", this);
    intro->setWordWrap(true);
    root->addWidget(intro);

    QPointer<QScrollArea> scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setObjectName("configurationEditorScroll");
    QPointer<QWidget> content = new QWidget(scroll);
    content->setMinimumWidth(0);
    content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QPointer<QVBoxLayout> stack = new QVBoxLayout(content);
    stack->setContentsMargins(8, 8, 8, 8);
    stack->setSpacing(10);
    content->setLayout(stack);
    scroll->setWidget(content);
    _content = content;
    _stack = stack;
    root->addWidget(scroll, 1);
    buildTiles(config);
    QPointer<QDialogButtonBox> buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttons->addButton("Apply", QDialogButtonBox::AcceptRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    root->addWidget(buttons);
}

SimulationConfig ConfigurationEditor::configuration() const
{
    SimulationConfig result = _configuration;
    readValues(result);
    return result;
}

void ConfigurationEditor::reload(const SimulationConfig& config)
{
    _configuration = config;
    _fields.clear();
    if (_stack == nullptr)
        return;
    for (;;) {
        std::unique_ptr<QLayoutItem> item(_stack->takeAt(0));
        if (item == nullptr)
            break;
        std::unique_ptr<QWidget> widget(item->widget());
    }
    buildTiles(config);
}

QPointer<QFormLayout> ConfigurationEditor::addTile(QPointer<QVBoxLayout> stack,
                                                   const QString& section, const QString& title,
                                                   const QString& description)
{
    QPointer<QGroupBox> tile = new QGroupBox(title, stack->parentWidget());
    tile->setMinimumWidth(0);
    tile->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    tile->setProperty("configTile", true);
    tile->setProperty("configSection", section);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(tile);
    QPointer<QLabel> hint = new QLabel(description, tile);
    hint->setWordWrap(true);
    hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    hint->setObjectName("configurationTileHint");
    layout->addWidget(hint);
    QPointer<QFormLayout> form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addLayout(form);
    stack->addWidget(tile);
    return form;
}

void ConfigurationEditor::buildTiles(const SimulationConfig& config)
{
    if (_stack == nullptr)
        return;
    addRuntimeTiles(config);
    _stack->addStretch(1);
}

void ConfigurationEditor::addRuntimeTiles(const SimulationConfig& config)
{
    QPointer<QFormLayout> advanced = addTile(
        _stack, "advanced", "Advanced I/O", "Only settings not exposed by Physics, Scene or Render.");
    addText(advanced, _fields, QStringLiteral("input_file"), config.inputFile);
    addCombo(advanced, _fields, QStringLiteral("input_format"), config.inputFormat,
             {"auto", "bin", "vtk", "vtk_binary", "xyz"});
    addText(advanced, _fields, QStringLiteral("export_directory"), config.exportDirectory);
    addCombo(advanced, _fields, QStringLiteral("export_format"), config.exportFormat,
             {"vtk", "vtk_binary", "xyz", "bin"});
    addInt(advanced, _fields, QStringLiteral("client_remote_command_timeout_ms"),
           static_cast<int>(config.clientRemoteCommandTimeoutMs), 1, 600000);
    addInt(advanced, _fields, QStringLiteral("client_remote_status_timeout_ms"),
           static_cast<int>(config.clientRemoteStatusTimeoutMs), 1, 600000);
    addInt(advanced, _fields, QStringLiteral("client_remote_snapshot_timeout_ms"),
           static_cast<int>(config.clientRemoteSnapshotTimeoutMs), 1, 600000);
    addInt(advanced, _fields, QStringLiteral("client_snapshot_queue_capacity"),
           static_cast<int>(config.clientSnapshotQueueCapacity), 1, 1024);
    addCombo(advanced, _fields, QStringLiteral("client_snapshot_drop_policy"),
             config.clientSnapshotDropPolicy,
             {"latest-only", "paced"});
}

void ConfigurationEditor::readValues(SimulationConfig& config) const
{
    readString(_fields, QStringLiteral("input_file"), config.inputFile);
    readString(_fields, QStringLiteral("input_format"), config.inputFormat);
    readInt(_fields, QStringLiteral("client_remote_command_timeout_ms"),
            config.clientRemoteCommandTimeoutMs);
    readInt(_fields, QStringLiteral("client_remote_status_timeout_ms"),
            config.clientRemoteStatusTimeoutMs);
    readInt(_fields, QStringLiteral("client_remote_snapshot_timeout_ms"),
            config.clientRemoteSnapshotTimeoutMs);
    readInt(_fields, QStringLiteral("client_snapshot_queue_capacity"),
            config.clientSnapshotQueueCapacity);
    readString(_fields, QStringLiteral("client_snapshot_drop_policy"),
               config.clientSnapshotDropPolicy);
    readString(_fields, QStringLiteral("export_directory"), config.exportDirectory);
    readString(_fields, QStringLiteral("export_format"), config.exportFormat);
    if (config.cosmologyEnabled && config.cosmologyMode == "comoving") {
        // This mode is a separate periodic PM solver, not a visual-expansion preset.
        config.initMode = "cosmology";
        config.presetStructure = "cosmology";
        // CUDA comoving PM is not qualified yet; selecting this GUI mode must keep a visible,
        // reproducible reference path rather than start a stalled renderer.
        config.solver = "octree_cpu";
        config.integrator = "leapfrog";
        config.cosmologyGeometry = "cube";
        config.treePmEnabled = true;
        config.treePmPreset = "pm_only";
        config.treePmModel = "pm_only";
        config.treePmAssignment = "tsc";
        config.treePmLocalGrid = false;
        config.sphEnabled = false;
        config.adaptiveTimeStepsEnabled = false;
    }
}
} // namespace bltzr_qt
