#ifndef BLITZAR_BLITZAR_H
#define BLITZAR_BLITZAR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(BLITZAR_SHARED)
#if defined(BLITZAR_BUILDING)
#define BLITZAR_API __declspec(dllexport)
#else
#define BLITZAR_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define BLITZAR_API __attribute__((visibility("default")))
#else
#define BLITZAR_API
#endif

typedef struct blitzar_context blitzar_context;

typedef enum blitzar_status {
    BLITZAR_STATUS_OK = 0,
    BLITZAR_STATUS_INVALID_ARGUMENT = 1,
    BLITZAR_STATUS_ALLOCATION_FAILURE = 2,
    BLITZAR_STATUS_INTERNAL_ERROR = 3,
    BLITZAR_STATUS_SINGULARITY = 4
} blitzar_status;

BLITZAR_API blitzar_status blitzar_context_create(
    blitzar_context** context);

BLITZAR_API void blitzar_context_destroy(blitzar_context* context);

BLITZAR_API blitzar_status blitzar_context_status(
    const blitzar_context* context);

BLITZAR_API const char* blitzar_status_message(blitzar_status status);

#ifdef __cplusplus
}
#endif

#endif
