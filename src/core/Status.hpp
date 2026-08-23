#ifndef BLITZAR_CORE_STATUS_HPP
#define BLITZAR_CORE_STATUS_HPP

#include <blitzar/blitzar.h>

namespace blitzar_core {

[[nodiscard]] constexpr blitzar_status ToPublicStatus(int value) noexcept
{
    switch (value) {
    case BLITZAR_STATUS_OK:

        return BLITZAR_STATUS_OK;

    case BLITZAR_STATUS_INVALID_ARGUMENT:

        return BLITZAR_STATUS_INVALID_ARGUMENT;

    case BLITZAR_STATUS_ALLOCATION_FAILURE:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case BLITZAR_STATUS_INTERNAL_ERROR:

        return BLITZAR_STATUS_INTERNAL_ERROR;

    case BLITZAR_STATUS_SINGULARITY:

        return BLITZAR_STATUS_SINGULARITY;

    case BLITZAR_STATUS_UNSUPPORTED:

        return BLITZAR_STATUS_UNSUPPORTED;

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace blitzar_core

#endif
