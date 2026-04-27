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
    /// Description: Executes the WorkspaceLayoutStore operation.
    explicit WorkspaceLayoutStore(std::string configPath);
    /// Description: Executes the deletePreset operation.
    bool deletePreset(const std::string& name) const;
    /// Description: Executes the loadPreset operation.
    bool loadPreset(const std::string& name, std::string& state, std::string& geometry) const;
    /// Description: Executes the listPresets operation.
    std::vector<std::string> listPresets() const;
    bool savePreset(const std::string& name, const std::string& state,
                    const std::string& geometry) const;

private:
    /// Description: Executes the normalizeName operation.
    static std::string normalizeName(const std::string& name);
    /// Description: Executes the presetPath operation.
    std::filesystem::path presetPath(const std::string& name) const;
    /// Description: Executes the readValueLine operation.
    static bool readValueLine(const std::string& line, const char* prefix, std::string& out);
    std::filesystem::path _layoutsRoot;
};
} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_
