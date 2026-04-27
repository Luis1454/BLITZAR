// File: runtime/src/command/CommandCatalog.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "command/CommandCatalog.hpp"
#include <sstream>

namespace grav_cmd {
/// Description: Describes the make spec operation contract.
static CommandSpec makeSpec(CommandId id, std::string name, std::string help, bool deterministic,
                            std::vector<CommandArgumentSpec> arguments)
{
    CommandSpec spec{};
    spec.id = id;
    spec.name = std::move(name);
    spec.help = std::move(help);
    spec.deterministic = deterministic;
    spec.arguments = std::move(arguments);
    return spec;
}

/// Description: Executes the all operation.
const std::vector<CommandSpec>& CommandCatalog::all()
{
    static const std::vector<CommandSpec> specs = {
        makeSpec(CommandId::Help, "help", "show available commands", true, {}),
        makeSpec(CommandId::LoadConfig, "load_config", "load config file and apply it", true,
                 {{"path", CommandArgumentKind::Path, false, {}}}),
        makeSpec(CommandId::Connect, "connect", "connect to remote server", true,
                 {{"host", CommandArgumentKind::Host, false, {}},
                  {"port", CommandArgumentKind::Port, false, {}}}),
        makeSpec(CommandId::Reconnect, "reconnect", "reconnect using current endpoint", true, {}),
        makeSpec(CommandId::Status, "status", "print current server status", true, {}),
        makeSpec(CommandId::Pause, "pause", "pause simulation", true, {}),
        makeSpec(CommandId::Resume, "resume", "resume simulation", true, {}),
        makeSpec(CommandId::Toggle, "toggle", "toggle pause state", true, {}),
        makeSpec(CommandId::Step, "step", "advance one or more steps", true,
                 {{"count", CommandArgumentKind::Uint, true, {}}}),
        makeSpec(CommandId::Reset, "reset", "reset simulation", true, {}),
        makeSpec(CommandId::Recover, "recover", "recover after a fault", true, {}),
        makeSpec(CommandId::Shutdown, "shutdown", "request server shutdown", true, {}),
        makeSpec(CommandId::SetDt, "set_dt", "set simulation dt in seconds", true,
                 {{"seconds", CommandArgumentKind::Float, false, {}}}),
        makeSpec(CommandId::SetSolver, "set_solver", "set solver mode", true,
                 {{"name",
                   CommandArgumentKind::Enum,
                   false,
                   {"pairwise_cuda", "octree_gpu", "octree_cpu"}}}),
        makeSpec(CommandId::SetIntegrator, "set_integrator", "set integrator mode", true,
                 {{"name", CommandArgumentKind::Enum, false, {"euler", "rk4"}}}),
        makeSpec(CommandId::SetProfile, "set_profile", "apply performance profile", true,
                 {{"name",
                   CommandArgumentKind::Enum,
                   false,
                   {"interactive", "balanced", "quality", "custom"}}}),
        makeSpec(CommandId::SetParticleCount, "set_particle_count", "set particle count", true,
                 {{"count", CommandArgumentKind::Uint, false, {}}}),
        makeSpec(
            CommandId::ExportSnapshot, "export_snapshot", "export snapshot to a file", true,
            {{"path", CommandArgumentKind::Path, false, {}},
             {"format", CommandArgumentKind::Enum, true, {"vtk", "vtk_binary", "xyz", "bin"}}}),
        makeSpec(CommandId::SaveCheckpoint, "save_checkpoint",
                 "save full restartable checkpoint to a file", true,
                 {{"path", CommandArgumentKind::Path, false, {}}}),
        makeSpec(CommandId::LoadCheckpoint, "load_checkpoint",
                 "load full restartable checkpoint from a file", true,
                 {{"path", CommandArgumentKind::Path, false, {}}}),
        makeSpec(CommandId::RunSteps, "run_steps",
                 "run deterministically for a fixed number of steps", true,
                 {{"count", CommandArgumentKind::Uint, false, {}}}),
        makeSpec(CommandId::RunUntil, "run_until", "run deterministically until simulated time",
                 true, {{"seconds", CommandArgumentKind::Float, false, {}}})};
    return specs;
}

/// Description: Executes the findByName operation.
const CommandSpec* CommandCatalog::findByName(std::string_view name)
{
    for (const CommandSpec& spec : all()) {
        if (spec.name == name)
            return &spec;
    }
    return nullptr;
}

/// Description: Executes the findById operation.
const CommandSpec* CommandCatalog::findById(CommandId id)
{
    for (const CommandSpec& spec : all()) {
        if (spec.id == id)
            return &spec;
    }
    return nullptr;
}

/// Description: Executes the renderHelp operation.
std::string CommandCatalog::renderHelp()
{
    std::ostringstream out;
    out << "commands:\n";
    for (const CommandSpec& spec : all()) {
        out << "  " << spec.name;
        for (const CommandArgumentSpec& argument : spec.arguments)
            out << " " << (argument.optional ? "[" : "<") << argument.name
                << (argument.optional ? "]" : ">");
        out << " : " << spec.help << "\n";
    }
    return out.str();
}
} // namespace grav_cmd
