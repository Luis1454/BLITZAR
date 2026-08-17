/*
 * @file engine/config/directive/src/directive/Write.cpp
 * @brief Public directive serialization facade.
 */

#include "directive/Write.hpp"

#include "WriteInternals.hpp"

#include "core/Config.hpp"

#include <ostream>

namespace bltzr_config {
void SimulationConfigDirective::write(std::ostream& out, const SimulationConfig& config)
{
    out << "# ==================================================\n";
    out << "# BLITZAR directive config\n";
    out << "# Generated automatically. Edit values then restart.\n";
    out << "# ==================================================\n\n";
    writeSimulation(out, config);
    writePerformance(out, config);
    writeAdaptive(out, config);
    writeOctree(out, config);
    writeTreePm(out, config);
    writePhysics(out, config);
    writeClient(out, config);
    writeExport(out, config);
    writeScene(out, config);
    writeSceneObjects(out, config);
    writePreset(out, config);
    writeThermal(out, config);
    writeGeneration(out, config);
    writeCentralBody(out, config);
    writeDisk(out, config);
    writeCloud(out, config);
    writeCosmology(out, config);
    writeTransform(out, config);
    writeSph(out, config);
    writeRender(out, config);
}
} // namespace bltzr_config
