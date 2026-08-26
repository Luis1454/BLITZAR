#ifndef BLITZAR_TESTS_FIXTURES_CHECK_HPP
#define BLITZAR_TESTS_FIXTURES_CHECK_HPP

#include <stdio.h>

#define BLITZAR_CHECK(condition) \
    do { \
        if (!(condition)) { \
            (void)fprintf(stderr, "[%s:%d] Check failed: %s\n", __FILE__, __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

#endif
