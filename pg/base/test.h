#pragma once
#include "base.h"

/*
    This implements a simple testing "framework".
    The testing framework is bootstraped by hand.
 */

/*
    How it works:

    Tests are organized in features (List, hashmap, subsytem)
    And each features can be tested in unit tests.

    A test feature is cancelled one of the subtests fails.
 */

typedef struct
{
    u32    total;
    u32    success;
} test_feature_effective_t;

#include <time.h>

#define TESTS()            b8 success_all = TRUE;
#define TESTS_END()        return !success_all;

#define TEST_INIT_RAND()   srand(time(NULL) * 7);
#define TEST_RND(min, max) ( ( rand() % (max - min + 1) ) + min )

#define TEST_DEF_FEATURE(feature_name, tests)           \
    test_feature_effective_t feature_ ## feature_name() \
    {                                                   \
        char *_name                 = #feature_name;    \
        test_feature_effective_t _e = { 0 };            \
        (void)_name;                                    \
        do                                              \
        {                                               \
            tests                                       \
        }                                               \
        while (0);                                      \
        return _e;                                      \
    }
#define TEST_DEF_SUBTEST(subtest_name, test) \
    b8 subtest_ ## subtest_name()            \
    {                                        \
        b8 _success = TRUE;                  \
        char *_name = #subtest_name;         \
        (void)_success;                      \
        (void)_name;                         \
        test                                 \
    }

#define TEST_SUBTEST(subtest_name)                                                                       \
    _e.total++;                                                                                          \
    if ( !subtest_ ## subtest_name() )                                                                   \
    {                                                                                                    \
        ERROR("Subtest '%s' failed, in feature '%s', %s:%d.", #subtest_name, _name, __FILE__, __LINE__); \
    }                                                                                                    \
    else                                                                                                 \
    {                                                                                                    \
        _e.success++;                                                                                    \
    }

#define TEST_FEATURE(feature_name)                                                       \
    do                                                                                   \
    {                                                                                    \
        test_feature_effective_t e = feature_ ## feature_name();                         \
        if (e.success < e.total)                                                         \
        {                                                                                \
            ERROR("Tests in '%s' failed : (%d/%d).", #feature_name, e.success, e.total); \
            success_all = FALSE;                                                         \
        }                                                                                \
        else                                                                             \
        {                                                                                \
            INFO("Test '%s' success (%d/%d).", #feature_name, e.total, e.total);         \
        }                                                                                \
    }                                                                                    \
    while (0);

#define TEST_ASSERT(cond, msg, ...)                                   \
    if (cond)                                                         \
    {                                                                 \
    }                                                                 \
    else                                                              \
    {                                                                 \
        ERROR("Test assertion failed !");                             \
        ERROR("\tstatement='%s' (%s:%d)", #cond, __FILE__, __LINE__); \
        ERROR("\tmessage='" #msg "'", ## __VA_ARGS__);                \
        _success = FALSE;                                             \
        goto subtest_cleanup;                                         \
    }

#define TEST_END(clean)                                                 \
   subtest_cleanup:                                                     \
    {                                                                   \
        do                                                              \
        {                                                               \
            clean return _success;                                      \
            goto subtest_cleanup; /*So that unused error does not pop*/ \
        }                                                               \
        while (0);                                                      \
    }

