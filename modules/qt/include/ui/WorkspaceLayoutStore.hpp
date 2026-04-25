#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_

#include <filesystem>
#include <string>
#include <vector>

namespace grav_qt {
class WorkspaceLayoutStore final {
public:
    explicit WorkspaceLayoutStore(std::string configPath);
    bool deletePreset(const std::string& name) const;
    bool loadPreset(const std::string& name, std::string& state, std::string& geometry) const;
    std::vector<std::string> listPresets() const;
    bool savePreset(const std::string& name, const std::string& state,
                    const std::string& geometry) const;

private:
    static std::string normalizeName(const std::string& name);
    std::filesystem::path presetPath(const std::string& name) const;
    static bool readValueLine(const std::string& line, const char* prefix, std::string& out);
    std::filesystem::path _layoutsRoot;
};
} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_WORKSPACELAYOUTSTORE_HPP_
