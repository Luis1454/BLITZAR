// File: modules/qt/module.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
#include "client/ClientRuntime.hpp"
#include "config/SimulationConfig.hpp"
#include "config/TextParse.hpp"
#include "ui/MainWindow.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMetaObject>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
static bool parseBool(std::string_view raw, bool& out)
{
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "1" || value == "true" || value == "on" || value == "yes") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "off" || value == "no") {
        out = false;
        return true;
    }
    return false;
}
static std::string trim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(),
                                        [](unsigned char c) { return std::isspace(c) != 0; });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}
static std::vector<std::string> splitTokens(const std::string& line)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (char c : line) {
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
            continue;
        }
        if (inQuotes && c == quoteChar) {
            inQuotes = false;
            quoteChar = '\0';
            continue;
        }
        if (!inQuotes && std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}
struct QtInProcState {
    std::string configPath = "simulation.ini";
    grav_client::ClientTransportArgs transport{"127.0.0.1",
                                               4545u,
                                               false, // autostart off by default
                                               {},
                                               {},
                                               grav_client::kClientRemoteCommandTimeoutMsDefault,
                                               grav_client::kClientRemoteStatusTimeoutMsDefault,
                                               grav_client::kClientRemoteSnapshotTimeoutMsDefault};
    std::thread uiThread;
    std::atomic<bool> running{false};
    std::atomic<bool> startupDone{false};
    std::atomic<bool> startupOk{false};
    std::string startupError;
    std::mutex startupMutex;
};
static void configureQtPluginPathFallback()
{
    QString pluginRoot;
    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        const QString localPlatformsDir = QDir(appDir).filePath("platforms");
        if (QFileInfo::exists(localPlatformsDir)) {
            pluginRoot = appDir;
        }
    }
    if (pluginRoot.isEmpty()) {
        pluginRoot = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    }
    if (pluginRoot.isEmpty()) {
        return;
    }
    const QString platformsDir = QDir(pluginRoot).filePath("platforms");
    if (QFileInfo::exists(platformsDir)) {
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", platformsDir.toUtf8());
    }
    qputenv("QT_PLUGIN_PATH", pluginRoot.toUtf8());
}
static void qtThreadMain(QtInProcState* state)
{
    try {
        int argc = 1;
        std::string appName = "gravityQtInProc";
        std::vector<char> appNameBuffer(appName.begin(), appName.end());
        appNameBuffer.push_back('\0');
        char* argv[] = {appNameBuffer.data(), nullptr};
        QApplication app(argc, argv);
        configureQtPluginPathFallback();
        SimulationConfig config = SimulationConfig::loadOrCreate(state->configPath);
        state->transport.remoteCommandTimeoutMs =
            grav_client::clampClientRemoteTimeoutMs(config.clientRemoteCommandTimeoutMs);
        state->transport.remoteStatusTimeoutMs =
            grav_client::clampClientRemoteTimeoutMs(config.clientRemoteStatusTimeoutMs);
        state->transport.remoteSnapshotTimeoutMs =
            grav_client::clampClientRemoteTimeoutMs(config.clientRemoteSnapshotTimeoutMs);
        auto runtime =
            std::make_unique<grav_client::ClientRuntime>(state->configPath, state->transport);
        grav_qt::MainWindow window(config, state->configPath, std::move(runtime));
        window.show();
        state->running.store(true);
        {
            std::lock_guard<std::mutex> lock(state->startupMutex);
            state->startupOk.store(true);
            state->startupDone.store(true);
        }
        (void)app.exec();
    }
    catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(state->startupMutex);
        state->startupOk.store(false);
        state->startupDone.store(true);
        state->startupError = ex.what();
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(state->startupMutex);
        state->startupOk.store(false);
        state->startupDone.store(true);
        state->startupError = "unknown Qt exception";
    }
    state->running.store(false);
}
static bool startQtUi(QtInProcState& state, const grav_client::ErrorBufferView& errorBuffer)
{
    if (state.uiThread.joinable() || state.running.load()) {
        return true;
    }
    state.startupDone.store(false);
    state.startupOk.store(false);
    state.startupError.clear();
    state.uiThread = std::thread(qtThreadMain, &state);
    // Wait briefly for startup signal.
    for (int i = 0; i < 200; ++i) {
        if (state.startupDone.load()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!state.startupDone.load()) {
        // Consider UI thread alive and starting.
        return true;
    }
    if (!state.startupOk.load()) {
        const std::string err =
            state.startupError.empty() ? "failed to initialize Qt module" : state.startupError;
        errorBuffer.write(err);
        if (state.uiThread.joinable()) {
            state.uiThread.join();
        }
        return false;
    }
    return true;
}
static void stopQtUi(QtInProcState& state)
{
    if (state.uiThread.joinable()) {
        QCoreApplication* app = QCoreApplication::instance();
        if (app != nullptr) {
            QMetaObject::invokeMethod(app, "quit", Qt::QueuedConnection);
        }
        state.uiThread.join();
    }
    state.running.store(false);
}
static void printHelp()
{
    std::cout << "[module-qt] commands:\n"
              << "  help\n"
              << "  status\n"
              << "  set-endpoint <host> <port>\n"
              << "  set-autostart <true|false>\n"
              << "  set-server-bin <path>\n"
              << "  restart\n"
              << "  wait\n";
}
class QtModuleBoundary final {
public:
    static bool create(const grav_module::ClientHostContextV1* context,
                       const grav_module::ClientModuleStateSlot& outModuleState,
                       const grav_client::ErrorBufferView& errorBuffer)
    {
        try {
            if (!outModuleState.isAvailable()) {
                errorBuffer.write("outModuleState is null");
                return false;
            }
            std::unique_ptr<QtInProcState> state = std::make_unique<QtInProcState>();
            if (context != nullptr && context->configPath != nullptr) {
                state->configPath = context->configPath;
            }
            return outModuleState.assign(
                grav_module::ClientModuleOpaqueState::fromRawPointer(state.release()));
        }
        catch (const std::exception& ex) {
            errorBuffer.write(ex.what());
            return false;
        }
        catch (...) {
            errorBuffer.write("unknown module create error");
            return false;
        }
    }
    static QtInProcState* requireState(grav_module::ClientModuleOpaqueState moduleState,
                                       const grav_client::ErrorBufferView& errorBuffer)
    {
        QtInProcState* state = static_cast<QtInProcState*>(moduleState.rawPointer());
        if (state == nullptr) {
            errorBuffer.write("module state is null");
        }
        return state;
    }
    static void destroy(grav_module::ClientModuleOpaqueState moduleState)
    {
        try {
            std::unique_ptr<QtInProcState> state(
                static_cast<QtInProcState*>(moduleState.rawPointer()));
            if (state != nullptr) {
                stopQtUi(*state);
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "[module-qt] destroy error: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[module-qt] destroy error: unknown\n";
        }
    }
    static bool start(grav_module::ClientModuleOpaqueState moduleState,
                      const grav_client::ErrorBufferView& errorBuffer)
    {
        try {
            QtInProcState* state = requireState(moduleState, errorBuffer);
            if (state == nullptr)
                return false;
            return startQtUi(*state, errorBuffer);
        }
        catch (const std::exception& ex) {
            errorBuffer.write(ex.what());
            return false;
        }
        catch (...) {
            errorBuffer.write("unknown module start error");
            return false;
        }
    }
    static void stop(grav_module::ClientModuleOpaqueState moduleState)
    {
        try {
            QtInProcState* state = static_cast<QtInProcState*>(moduleState.rawPointer());
            if (state != nullptr) {
                stopQtUi(*state);
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "[module-qt] stop error: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[module-qt] stop error: unknown\n";
        }
    }
};
extern "C" GRAVITY_CLIENT_MODULE_EXPORT_ATTR const grav_module::ClientModuleExportsV1*
gravity_client_module_v1()
{
    static const grav_module::ClientModuleExportsV1 exports{
        grav_module::kClientModuleApiVersionV1,
        "qt",
        [](const grav_module::ClientHostContextV1* context, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return QtModuleBoundary::create(
                context, grav_module::ClientModuleStateSlot(outModuleState),
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            QtModuleBoundary::destroy(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return QtModuleBoundary::start(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState),
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            QtModuleBoundary::stop(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            const grav_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            const grav_module::ClientModuleCommandControl commandControl(outKeepRunning);
            try {
                QtInProcState* state = QtModuleBoundary::requireState(
                    grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState), errorView);
                if (state == nullptr)
                    return false;
                commandControl.setContinue();
                const std::string line =
                    trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                const std::vector<std::string> tokens = splitTokens(line);
                if (tokens.empty()) {
                    return true;
                }
                const std::string& cmd = tokens[0];
                if (cmd == "help") {
                    printHelp();
                    return true;
                }
                if (cmd == "quit" || cmd == "exit") {
                    commandControl.requestStop();
                    return true;
                }
                if (cmd == "status") {
                    std::cout << "[module-qt] running=" << (state->running.load() ? "yes" : "no")
                              << " endpoint=" << state->transport.remoteHost << ":"
                              << state->transport.remotePort
                              << " autostart=" << (state->transport.remoteAutoStart ? "on" : "off");
                    if (!state->transport.serverExecutable.empty()) {
                        std::cout << " server_bin=" << state->transport.serverExecutable;
                    }
                    std::cout << "\n";
                    return true;
                }
                if (cmd == "wait") {
                    while (!state->startupDone.load() || state->running.load()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    commandControl.requestStop();
                    return true;
                }
                if (cmd == "set-endpoint") {
                    if (tokens.size() < 3u) {
                        errorView.write("usage: set-endpoint <host> <port>");
                        return false;
                    }
                    unsigned int parsedPort = 0u;
                    if (!grav_text::parseNumber(tokens[2], parsedPort) || parsedPort == 0u ||
                        parsedPort > 65535u) {
                        errorView.write("invalid port");
                        return false;
                    }
                    state->transport.remoteHost = tokens[1];
                    state->transport.remotePort = static_cast<std::uint16_t>(parsedPort);
                    std::cout << "[module-qt] endpoint set (restart required)\n";
                    return true;
                }
                if (cmd == "set-autostart") {
                    if (tokens.size() < 2u) {
                        errorView.write("usage: set-autostart <true|false>");
                        return false;
                    }
                    bool value = false;
                    if (!parseBool(tokens[1], value)) {
                        errorView.write("invalid bool value");
                        return false;
                    }
                    state->transport.remoteAutoStart = value;
                    std::cout << "[module-qt] autostart set (restart required)\n";
                    return true;
                }
                if (cmd == "set-server-bin") {
                    if (tokens.size() < 2u) {
                        errorView.write("usage: set-server-bin <path>");
                        return false;
                    }
                    state->transport.serverExecutable = tokens[1];
                    std::cout << "[module-qt] server bin set (restart required)\n";
                    return true;
                }
                if (cmd == "restart") {
                    stopQtUi(*state);
                    return startQtUi(*state, errorView);
                }
                errorView.write("unknown module command");
                return false;
            }
            catch (const std::exception& ex) {
                errorView.write(ex.what());
                return false;
            }
            catch (...) {
                errorView.write("unknown module command error");
                return false;
            }
        }};
    return &exports;
}
