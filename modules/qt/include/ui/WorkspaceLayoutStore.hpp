// File: modules/qt/include/ui/WorkspaceLayoutStore.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_

#include <filesystem>
#include <string>
#include <vector>

namespace grav_qt {
/// Description: Defines the WorkspaceLayoutStore data or behavior contract.
class WorkspaceLayoutStore final {
public:
    /// Description: Describes the workspace layout store operation contract.
    explicit WorkspaceLayoutStore(std::string configPath);
    /// Description: Describes the delete preset operation contract.
    bool deletePreset(const std::string& name) const;
    /// Description: Describes the load preset operation contract.
    bool loadPreset(const std::string& name, std::string& state, std::string& geometry) const;
    /// Description: Describes the list presets operation contract.
    std::vector<std::string> listPresets() const;
    /// Description: Describes the save preset operation contract.
    bool savePreset(const std::string& name, const std::string& state,
                    const std::string& geometry) const;

private:
    /// Description: Describes the normalize name operation contract.
    static std::string normalizeName(const std::string& name);
    /// Description: Describes the preset path operation contract.
    std::filesystem::path presetPath(const std::string& name) const;
    /// Description: Describes the read value line operation contract.
    static bool readValueLine(const std::string& line, const char* prefix, std::string& out);
    std::filesystem::path _layoutsRoot;
};
} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_
