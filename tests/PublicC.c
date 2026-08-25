#include <blitzar/blitzar.h>

_Static_assert(sizeof(blitzar_status) == sizeof(int32_t), "public C ABI status width");

int PublicCCompileProbe(void)
{
    return BLITZAR_STATUS_OK;
}
