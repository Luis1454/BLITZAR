/*
 * @file modules/qt/ui/WorkspaceLayoutStore.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/WorkspaceLayoutStore.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace bltzr_qt {

WorkspaceLayoutStore::WorkspaceLayoutStore(std::string configPath)
{
    std::filesystem::path basePath = configPath.empty()
                                         ? std::filesystem::current_path()
                                         : std::filesystem::path(std::move(configPath));
    if (basePath.has_filename()) {
        basePath = basePath.parent_path();
    }
    if (basePath.empty()) {
        basePath = std::filesystem::current_path();
    }
    _layoutsRoot = basePath / "workspace_layouts" / "qt";
}

bool WorkspaceLayoutStore::deletePreset(const std::string& name) const
{
    const std::filesystem::path path = presetPath(name);
    std::error_code ec;
    return std::filesystem::remove(path, ec) || !std::filesystem::exists(path);
}

bool WorkspaceLayoutStore::loadPreset(const std::string& name, std::string& state,
                                      std::string& geometry) const
{
    const std::filesystem::path path = presetPath(name);
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    bool stateLoaded = false;
    bool geometryLoaded = false;
    while (std::getline(in, line)) {
        stateLoaded = stateLoaded || readValueLine(line, "state=", state);
        geometryLoaded = geometryLoaded || readValueLine(line, "geometry=", geometry);
    }
    return stateLoaded && geometryLoaded;
}

std::vector<std::string> WorkspaceLayoutStore::listPresets() const
{
    std::vector<std::string> presets;
    std::error_code ec;
    if (!std::filesystem::exists(_layoutsRoot, ec)) {
        return presets;
    }
    for (const auto& entry : std::filesystem::directory_iterator(_layoutsRoot, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".layout") {
            continue;
        }
        presets.push_back(entry.path().stem().string());
    }
    std::sort(presets.begin(), presets.end());
    return presets;
}

bool WorkspaceLayoutStore::savePreset(const std::string& name, const std::string& state,
                                      const std::string& geometry) const
{
    const std::filesystem::path path = presetPath(name);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "state=" << state << "\n";
    out << "geometry=" << geometry << "\n";
    return out.good();
}

std::string WorkspaceLayoutStore::normalizeName(const std::string& name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (char raw : name) {
        const unsigned char ch = static_cast<unsigned char>(raw);
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
            continue;
        }
        if (raw == '-' || raw == '_') {
            normalized.push_back(raw);
            continue;
        }
        if (std::isspace(ch) != 0) {
            normalized.push_back('_');
        }
    }
    while (!normalized.empty() && normalized.front() == '_') {
        normalized.erase(normalized.begin());
    }
    while (!normalized.empty() && normalized.back() == '_') {
        normalized.pop_back();
    }
    return normalized;
}

std::filesystem::path WorkspaceLayoutStore::presetPath(const std::string& name) const
{
    const std::string normalized = normalizeName(name);
    return _layoutsRoot / ((normalized.empty() ? std::string("default") : normalized) + ".layout");
}

bool WorkspaceLayoutStore::readValueLine(const std::string& line, const char* prefix,
                                         std::string& out)
{
    const std::string expected(prefix);
    if (line.rfind(expected, 0) != 0) {
        return false;
    }
    out = line.substr(expected.size());
    return !out.empty();
}
} // namespace bltzr_qt
