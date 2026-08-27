#ifndef BLITZAR_TESTS_FIXTURES_CHECK_HPP
#define BLITZAR_TESTS_FIXTURES_CHECK_HPP

#include <stdio.h>

#if defined(_MSC_VER) && !defined(__clang__)
#define BLITZAR_STATIC_CHECK(condition, name) typedef char name[(condition) ? 1 : -1]
#else
#define BLITZAR_STATIC_CHECK(condition, name) _Static_assert(condition, #name)
#endif

#define BLITZAR_CHECK(condition) \
    do { \
        if (!(condition)) { \
            (void)fprintf(stderr, "[%s:%d] Check failed: %s\n", __FILE__, __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

#endif
