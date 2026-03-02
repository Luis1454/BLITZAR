#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

ModuleState::ModuleState()
    : client()
    , host("127.0.0.1")
    , port(4545u)
{
}

} // namespace grav_module_cli
