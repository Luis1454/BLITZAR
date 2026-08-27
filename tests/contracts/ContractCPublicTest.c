#include "fixtures/FixtureCheck.hpp"

#include <blitzar/blitzar.h>

BLITZAR_STATIC_CHECK(sizeof(blitzar_status) == sizeof(int32_t), blitzar_public_status_width);

int PublicCCompileProbe(void)
{
    return BLITZAR_STATUS_OK;
}
