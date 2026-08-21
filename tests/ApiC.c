#include <blitzar/blitzar.h>

#include <assert.h>

int main(void)
{
    blitzar_context* context = NULL;
    assert(blitzar_context_create(&context) == BLITZAR_STATUS_OK);
    assert(context != NULL);
    assert(blitzar_context_status(context) == BLITZAR_STATUS_OK);
    assert(blitzar_context_status(NULL) == BLITZAR_STATUS_INVALID_ARGUMENT);
    assert(blitzar_context_create(NULL) == BLITZAR_STATUS_INVALID_ARGUMENT);
    assert(blitzar_status_message(BLITZAR_STATUS_OK)[0] == 'o');
    blitzar_context_destroy(context);
    return 0;
}
