#ifndef BLITZAR_IO_POSTPROCESS_POST_PROCESS_STATE_HPP
#define BLITZAR_IO_POSTPROCESS_POST_PROCESS_STATE_HPP

#include "io/diagnostics/ConservationCsv.hpp"
#include "io/postprocess/PostProcessInput.hpp"

#include <blitzar/blitzar.h>

namespace blitzar_io {

[[nodiscard]] blitzar_status ProcessSnapshots(
    const PostProcessInput& input, ConservationCsv& output) noexcept;

} // namespace blitzar_io

#endif
