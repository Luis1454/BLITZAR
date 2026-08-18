/*
 * @file engine/server/simulation/runtime/SrvRuntimeBase.hpp
 * @brief Shared includes for internal simulation-server contracts.
 */

#ifndef BLITZAR_ENGINE_SERVER_SIMULATION_RUNTIME_SRV_RUNTIME_BASE_HPP_
#define BLITZAR_ENGINE_SERVER_SIMULATION_RUNTIME_SRV_RUNTIME_BASE_HPP_

#include "config/env/platform/CfgBase.hpp"
#include "config/modes/normalization/CfgNormalize.hpp"
#include "config/profile/profile/CfgPerformance.hpp"
#include "config/profile/profile/CfgMain.hpp"
#include "platform/paths/PltPaths.hpp"
#include "protocol/PtcProtocol.hpp"
#include "server/simulation/configuration/SrvSimulationInitConfig.hpp"
#include "server/simulation/runtime/SrvSimulationServer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#if BLITZAR_ENABLE_CUDA
#include <cuda_runtime.h>
#endif
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#endif
