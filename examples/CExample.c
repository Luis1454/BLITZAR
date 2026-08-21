#include <blitzar/blitzar.h>

#include <stddef.h>

int main(void)
{
    blitzar_context* context = NULL;
    const blitzar_status status = blitzar_context_create(&context);
    if (status != BLITZAR_STATUS_OK) {
        return (int)status;
    }
    blitzar_context_destroy(context);
    return 0;
}
