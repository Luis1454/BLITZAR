#include "sim/SimulationArgs.hpp"
#include "sim/SimulationConfig.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>

#include <iostream>

int main(int argc, char **argv)
{
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(argc, argv, "simulation.ini");
    SimulationConfig config = SimulationConfig::loadOrCreate(runtime.configPath);
    applyArgsToConfig(argc, argv, config, runtime, std::cerr);
    if (runtime.showHelp) {
        printUsage(std::cout, (argc > 0 && argv[0]) ? argv[0] : "myAppQt.exe", false);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }

    QApplication app(argc, argv);
    qtui::MainWindow window(config, runtime.configPath);
    window.show();
    return app.exec();
}
