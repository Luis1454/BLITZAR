#ifndef BLITZAR_IO_POSTPROCESS_POST_PROCESS_STATE_HPP
#define BLITZAR_IO_POSTPROCESS_POST_PROCESS_STATE_HPP

#include "io/diag/ConservationCsv.hpp"
#include "io/post/input/PostProcessInput.hpp"

#include <blitzar/c/blitzar.h>

namespace blitzar_io {

[[nodiscard]] blitzar_status ProcessSnapshots(
    const PostProcessInput& input, ConservationCsv& output) noexcept;

} // namespace blitzar_io

#endif
