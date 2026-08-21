#include <blitzar/blitzar.h>

#include <new>

struct blitzar_context {
    blitzar_status status;
};

extern "C" blitzar_status blitzar_context_create(
    blitzar_context** context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    *context = nullptr;
    try {
        *context = new blitzar_context{BLITZAR_STATUS_OK};
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    return BLITZAR_STATUS_OK;
}

extern "C" void blitzar_context_destroy(blitzar_context* context)
{
    delete context;
}

extern "C" blitzar_status blitzar_context_status(
    const blitzar_context* context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return context->status;
}

extern "C" const char* blitzar_status_message(blitzar_status status)
{
    switch (status) {
    case BLITZAR_STATUS_OK:
        return "ok";
    case BLITZAR_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case BLITZAR_STATUS_ALLOCATION_FAILURE:
        return "allocation failure";
    case BLITZAR_STATUS_INTERNAL_ERROR:
        return "internal error";
    case BLITZAR_STATUS_SINGULARITY:
        return "gravitational singularity";
    default:
        return "unknown status";
    }
}
