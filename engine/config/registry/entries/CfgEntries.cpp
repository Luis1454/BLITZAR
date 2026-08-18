/*
 * @file engine/config/registry/entries/CfgEntries.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Ordered composition of the private simulation option tables.
 */

#include "config/registry/runtime/CfgInternal.hpp"

namespace bltzr_config {

extern const SimulationOptionEntry kCorePrimaryOptions[];
extern const std::size_t kCorePrimaryOptionCount;
extern const SimulationOptionEntry kClientPrimaryOptions[];
extern const std::size_t kClientPrimaryOptionCount;
extern const SimulationOptionEntry kInitStateOptions[];
extern const std::size_t kInitStateOptionCount;
extern const SimulationOptionEntry kFluidPrimaryOptions[];
extern const std::size_t kFluidPrimaryOptionCount;
extern const SimulationOptionEntry kCoreTailOptions[];
extern const std::size_t kCoreTailOptionCount;
extern const SimulationOptionEntry kFluidTailOptions[];
extern const std::size_t kFluidTailOptionCount;
extern const SimulationOptionEntry kClientTailOptions[];
extern const std::size_t kClientTailOptionCount;

const SimulationOptionRange kSimulationOptionRanges[] = {
    {kCorePrimaryOptions[0], kCorePrimaryOptionCount},
    {kClientPrimaryOptions[0], kClientPrimaryOptionCount},
    {kInitStateOptions[0], kInitStateOptionCount},
    {kFluidPrimaryOptions[0], kFluidPrimaryOptionCount},
    {kCoreTailOptions[0], kCoreTailOptionCount},
    {kFluidTailOptions[0], kFluidTailOptionCount},
    {kClientTailOptions[0], kClientTailOptionCount},
};

const std::size_t kSimulationOptionRangeCount =
    sizeof(kSimulationOptionRanges) / sizeof(kSimulationOptionRanges[0]);

} // namespace bltzr_config
