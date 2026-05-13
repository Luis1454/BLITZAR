/*
 * @file runtime/src/command/catalog/Catalog.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "command/catalog/Catalog.hpp"
#include <sstream>

namespace bltzr_cmd {
static Spec makeSpec(Id id, std::string name, std::string help, bool deterministic,
                            std::vector<ArgumentSpec> arguments)
{
    Spec spec{};
    spec.id = id;
    spec.name = std::move(name);
    spec.help = std::move(help);
    spec.deterministic = deterministic;
    spec.arguments = std::move(arguments);
    return spec;
}

const std::vector<Spec>& Catalog::all()
{
    static const std::vector<Spec> specs = {
        makeSpec(Id::Help, "help", "show available commands", true, {}),
        makeSpec(Id::LoadConfig, "load_config", "load config file and apply it", true,
                 {{"path", ArgumentKind::Path, false, {}}}),
        makeSpec(Id::Connect, "connect", "connect to remote server", true,
                 {{"host", ArgumentKind::Host, false, {}},
                  {"port", ArgumentKind::Port, false, {}}}),
        makeSpec(Id::Reconnect, "reconnect", "reconnect using current endpoint", true, {}),
        makeSpec(Id::Status, "status", "print current server status", true, {}),
        makeSpec(Id::Pause, "pause", "pause simulation", true, {}),
        makeSpec(Id::Resume, "resume", "resume simulation", true, {}),
        makeSpec(Id::Toggle, "toggle", "toggle pause state", true, {}),
        makeSpec(Id::Step, "step", "advance one or more steps", true,
                 {{"count", ArgumentKind::Uint, true, {}}}),
        makeSpec(Id::Reset, "reset", "reset simulation", true, {}),
        makeSpec(Id::Recover, "recover", "recover after a fault", true, {}),
        makeSpec(Id::Shutdown, "shutdown", "request server shutdown", true, {}),
        makeSpec(Id::SetDt, "set_dt", "set simulation dt in seconds", true,
                 {{"seconds", ArgumentKind::Float, false, {}}}),
        makeSpec(Id::SetSolver, "set_solver", "set solver mode", true,
                 {{"name",
                   ArgumentKind::Enum,
                   false,
                   {"pairwise_cuda", "octree_gpu", "octree_cpu"}}}),
        makeSpec(Id::SetIntegrator, "set_integrator", "set integrator mode", true,
                 {{"name", ArgumentKind::Enum, false, {"euler", "rk4"}}}),
        makeSpec(Id::SetProfile, "set_profile", "apply performance profile", true,
                 {{"name",
                   ArgumentKind::Enum,
                   false,
                   {"interactive", "balanced", "quality", "custom"}}}),
        makeSpec(Id::SetParticleCount, "set_particle_count", "set particle count", true,
                 {{"count", ArgumentKind::Uint, false, {}}}),
        makeSpec(
            Id::ExportSnapshot, "export_snapshot", "export snapshot to a file", true,
            {{"path", ArgumentKind::Path, false, {}},
             {"format", ArgumentKind::Enum, true, {"vtk", "vtk_binary", "xyz", "bin"}}}),
        makeSpec(Id::SaveCheckpoint, "save_checkpoint",
                 "save full restartable checkpoint to a file", true,
                 {{"path", ArgumentKind::Path, false, {}}}),
        makeSpec(Id::LoadCheckpoint, "load_checkpoint",
                 "load full restartable checkpoint from a file", true,
                 {{"path", ArgumentKind::Path, false, {}}}),
        makeSpec(Id::RunSteps, "run_steps",
                 "run deterministically for a fixed number of steps", true,
                 {{"count", ArgumentKind::Uint, false, {}}}),
        makeSpec(Id::RunUntil, "run_until", "run deterministically until simulated time",
                 true, {{"seconds", ArgumentKind::Float, false, {}}})};
    return specs;
}

const Spec* Catalog::findByName(std::string_view name)
{
    for (const Spec& spec : all()) {
        if (spec.name == name)
            return &spec;
    }
    return nullptr;
}

const Spec* Catalog::findById(Id id)
{
    for (const Spec& spec : all()) {
        if (spec.id == id)
            return &spec;
    }
    return nullptr;
}

std::string Catalog::renderHelp()
{
    std::ostringstream out;
    out << "commands:\n";
    for (const Spec& spec : all()) {
        out << "  " << spec.name;
        for (const ArgumentSpec& argument : spec.arguments)
            out << " " << (argument.optional ? "[" : "<") << argument.name
                << (argument.optional ? "]" : ">");
        out << " : " << spec.help << "\n";
    }
    return out.str();
}
} // namespace bltzr_cmd
