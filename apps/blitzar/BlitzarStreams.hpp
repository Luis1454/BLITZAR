#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_STREAMS_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_STREAMS_HPP

#include "BlitzarSummary.hpp"

#include <iosfwd>

namespace blitzar_cli {

struct BlitzarStreams final {
    std::ostream& standard_output;
    std::ostream& standard_error;
    BlitzarOutputFormat format{BlitzarOutputFormat::Human};
};

} // namespace blitzar_cli

#endif
