#include "sim/ErrorBuffer.hpp"
#include "sim/FrontendModuleApi.hpp"
#include "sim/FrontendRuntime.hpp"
#include "sim/SimulationConfig.hpp"
#include "sim/TextParse.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace {

bool parseBool(std::string_view raw, bool &out)
{
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
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

std::string trim(const std::string &input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::vector<std::string> splitTokens(const std::string &line)
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
    sim::FrontendTransportArgs transport{
        true,           // remote mode
        "127.0.0.1",
        4545u,
        false,          // autostart off by default
        {}
    };
    std::thread uiThread;
    std::atomic<bool> running{false};
    std::atomic<bool> startupDone{false};
    std::atomic<bool> startupOk{false};
    std::string startupError;
    std::mutex startupMutex;
};

std::string currentExecutablePath()
{
#if defined(_WIN32)
    std::vector<char> buffer(4096u, '\0');
    const DWORD count = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (count == 0 || count >= buffer.size()) {
        return {};
    }
    return std::string(buffer.data(), buffer.data() + count);
#elif defined(__APPLE__)
    std::vector<char> buffer(4096u, '\0');
    std::uint32_t size = static_cast<std::uint32_t>(buffer.size());
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    return std::string(buffer.data());
#else
    std::vector<char> buffer(4096u, '\0');
    const ssize_t count = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1u);
    if (count <= 0) {
        return {};
    }
    buffer[static_cast<std::size_t>(count)] = '\0';
    return std::string(buffer.data());
#endif
}

void configureQtPluginPathFallback()
{
    const std::string exePath = currentExecutablePath();
    if (exePath.empty()) {
        return;
    }
    const QFileInfo exeInfo(QString::fromStdString(exePath));
    const QString appDir = exeInfo.absolutePath();
    const QString platformsDir = QDir(appDir).filePath("platforms");
    if (QFileInfo::exists(platformsDir)) {
        const QByteArray platformsUtf8 = platformsDir.toUtf8();
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM_PLUGIN_PATH")) {
            qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", platformsUtf8);
        }
        if (qEnvironmentVariableIsEmpty("QT_PLUGIN_PATH")) {
            qputenv("QT_PLUGIN_PATH", appDir.toUtf8());
        }
    }
}

void qtThreadMain(QtInProcState *state)
{
    try {
        int argc = 1;
        std::string appName = currentExecutablePath();
        if (appName.empty()) {
            appName = "gravityQtInProc";
        }
        std::vector<char> appNameBuffer(appName.begin(), appName.end());
        appNameBuffer.push_back('\0');
        char *argv[] = {appNameBuffer.data(), nullptr};

        configureQtPluginPathFallback();

        QApplication app(argc, argv);
        SimulationConfig config = SimulationConfig::loadOrCreate(state->configPath);
        state->transport.remoteCommandTimeoutMs = sim::clampFrontendRemoteTimeoutMs(config.frontendRemoteCommandTimeoutMs);
        state->transport.remoteStatusTimeoutMs = sim::clampFrontendRemoteTimeoutMs(config.frontendRemoteStatusTimeoutMs);
        state->transport.remoteSnapshotTimeoutMs = sim::clampFrontendRemoteTimeoutMs(config.frontendRemoteSnapshotTimeoutMs);
        auto runtime = std::make_unique<sim::FrontendRuntime>(state->configPath, state->transport);
        qtui::MainWindow window(config, state->configPath, std::move(runtime));
        window.show();

        {
            std::lock_guard<std::mutex> lock(state->startupMutex);
            state->startupOk.store(true);
            state->startupDone.store(true);
        }
        state->running.store(true);
        (void)app.exec();
    } catch (const std::exception &ex) {
        std::lock_guard<std::mutex> lock(state->startupMutex);
        state->startupOk.store(false);
        state->startupDone.store(true);
        state->startupError = ex.what();
    } catch (...) {
        std::lock_guard<std::mutex> lock(state->startupMutex);
        state->startupOk.store(false);
        state->startupDone.store(true);
        state->startupError = "unknown Qt exception";
    }
    state->running.store(false);
}

bool startQtUi(QtInProcState &state, char *errorBuffer, std::size_t errorBufferSize)
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
        const std::string err = state.startupError.empty() ? "failed to initialize Qt module" : state.startupError;
        sim::writeErrorBuffer(errorBuffer, errorBufferSize, err);
        if (state.uiThread.joinable()) {
            state.uiThread.join();
        }
        return false;
    }
    return true;
}

void stopQtUi(QtInProcState &state)
{
    if (state.uiThread.joinable()) {
        QCoreApplication *app = QCoreApplication::instance();
        if (app != nullptr) {
            QMetaObject::invokeMethod(app, "quit", Qt::QueuedConnection);
        }
        state.uiThread.join();
    }
    state.running.store(false);
}

void printHelp()
{
    std::cout
        << "[module-qt] commands:\n"
        << "  help\n"
        << "  status\n"
        << "  set-endpoint <host> <port>\n"
        << "  set-autostart <true|false>\n"
        << "  set-backend-bin <path>\n"
        << "  restart\n";
}

} // namespace

GRAVITY_FRONTEND_MODULE_EXPORT const sim::module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const sim::module::FrontendModuleExportsV1 exports{
        sim::module::kFrontendModuleApiVersionV1,
        "qt-inproc-module",
        [](const sim::module::FrontendModuleHostContextV1 *context, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                if (outModuleState == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "outModuleState is null");
                    return false;
                }
                auto *state = new QtInProcState();
                if (context != nullptr && context->configPath != nullptr) {
                    state->configPath = context->configPath;
                }
                *outModuleState = state;
                return true;
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module create error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                auto *state = static_cast<QtInProcState *>(moduleState);
                if (state != nullptr) {
                    stopQtUi(*state);
                    delete state;
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-qt] destroy error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-qt] destroy error: unknown\n";
            }
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<QtInProcState *>(moduleState);
                if (state == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                return startQtUi(*state, errorBuffer, errorBufferSize);
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module start error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                auto *state = static_cast<QtInProcState *>(moduleState);
                if (state != nullptr) {
                    stopQtUi(*state);
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-qt] stop error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-qt] stop error: unknown\n";
            }
        },
        [](void *moduleState, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<QtInProcState *>(moduleState);
                if (state == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                if (outKeepRunning != nullptr) {
                    *outKeepRunning = true;
                }
                const std::string line = trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                const std::vector<std::string> tokens = splitTokens(line);
                if (tokens.empty()) {
                    return true;
                }
                const std::string &cmd = tokens[0];
                if (cmd == "help") {
                    printHelp();
                    return true;
                }
                if (cmd == "quit" || cmd == "exit") {
                    if (outKeepRunning != nullptr) {
                        *outKeepRunning = false;
                    }
                    return true;
                }
                if (cmd == "status") {
                    std::cout << "[module-qt] running=" << (state->running.load() ? "yes" : "no")
                              << " endpoint=" << state->transport.remoteHost << ":" << state->transport.remotePort
                              << " autostart=" << (state->transport.remoteAutoStart ? "on" : "off");
                    if (!state->transport.backendExecutable.empty()) {
                        std::cout << " backend_bin=" << state->transport.backendExecutable;
                    }
                    std::cout << "\n";
                    return true;
                }
                if (cmd == "set-endpoint") {
                    if (tokens.size() < 3u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: set-endpoint <host> <port>");
                        return false;
                    }
                    unsigned int parsedPort = 0u;
                    if (!sim::text::parseNumber(tokens[2], parsedPort) || parsedPort == 0u || parsedPort > 65535u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid port");
                        return false;
                    }
                    state->transport.remoteHost = tokens[1];
                    state->transport.remotePort = static_cast<std::uint16_t>(parsedPort);
                    std::cout << "[module-qt] endpoint set (restart required)\n";
                    return true;
                }
                if (cmd == "set-autostart") {
                    if (tokens.size() < 2u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: set-autostart <true|false>");
                        return false;
                    }
                    bool value = false;
                    if (!parseBool(tokens[1], value)) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid bool value");
                        return false;
                    }
                    state->transport.remoteAutoStart = value;
                    std::cout << "[module-qt] autostart set (restart required)\n";
                    return true;
                }
                if (cmd == "set-backend-bin") {
                    if (tokens.size() < 2u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: set-backend-bin <path>");
                        return false;
                    }
                    state->transport.backendExecutable = tokens[1];
                    std::cout << "[module-qt] backend bin set (restart required)\n";
                    return true;
                }
                if (cmd == "restart") {
                    stopQtUi(*state);
                    return startQtUi(*state, errorBuffer, errorBufferSize);
                }

                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command");
                return false;
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command error");
                return false;
            }
        }
    };
    return &exports;
}
