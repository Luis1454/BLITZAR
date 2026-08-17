/*
 * @file engine/config/directive/src/directive/WriteInternals.hpp
 * @brief Private serialization contracts grouped by configuration concern.
 */

#ifndef BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_WRITEINTERNALS_HPP_
#define BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_WRITEINTERNALS_HPP_

#include <iosfwd>

struct SimulationConfig;

namespace bltzr_config {
void writeSimulation(std::ostream& out, const SimulationConfig& config);
void writePerformance(std::ostream& out, const SimulationConfig& config);
void writeAdaptive(std::ostream& out, const SimulationConfig& config);
void writeOctree(std::ostream& out, const SimulationConfig& config);
void writeTreePm(std::ostream& out, const SimulationConfig& config);
void writePhysics(std::ostream& out, const SimulationConfig& config);
void writeClient(std::ostream& out, const SimulationConfig& config);
void writeExport(std::ostream& out, const SimulationConfig& config);
void writeScene(std::ostream& out, const SimulationConfig& config);
void writeSceneObjects(std::ostream& out, const SimulationConfig& config);
void writePreset(std::ostream& out, const SimulationConfig& config);
void writeThermal(std::ostream& out, const SimulationConfig& config);
void writeGeneration(std::ostream& out, const SimulationConfig& config);
void writeCentralBody(std::ostream& out, const SimulationConfig& config);
void writeDisk(std::ostream& out, const SimulationConfig& config);
void writeCloud(std::ostream& out, const SimulationConfig& config);
void writeCosmology(std::ostream& out, const SimulationConfig& config);
void writeTransform(std::ostream& out, const SimulationConfig& config);
void writeSph(std::ostream& out, const SimulationConfig& config);
void writeRender(std::ostream& out, const SimulationConfig& config);
} // namespace bltzr_config

#endif // BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_WRITEINTERNALS_HPP_
