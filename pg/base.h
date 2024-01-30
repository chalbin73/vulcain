#pragma once

//NOTE: Contains definitions to help identify the compiler used


#ifdef __clang__
    #define COMP_CLANG 1
    #define COMP_GCC   0
#elif defined(__GNUC__)
    #define COMP_CLANG 0
    #define COMP_GCC   1
#else
    #error "Compiler not supported"
#endif

#include <assert.h>

#if COMP_CLANG
    #define STATIC_ASSERT(a, m) static_assert(a, m)

    #if __has_builtin(__builtin_debugtrap)
        #define DEBUG_BRK()     __builtin_debugtrap()
    #else
        #include <stdlib.h>
        #define DEBUG_BRK()     abort()
    #endif
#elif COMP_GCC
    #define STATIC_ASSERT(a, m) _Static_assert(a, m)

    #if defined(SIGTRAP)
        #if SYSTEM_POSIX
            #define DEBUG_BRK() raise(SIGTRAP)
        #endif
    #else
        #include <stdlib.h>
        #define DEBUG_BRK()     abort()
    #endif
#endif


//NOTE: This header file contains defines used to determine the system on which this program is being compiled on

// Is a posix system ?
#if defined(__unix__) || (defined (__APPLE__) && defined ( __MACH__) )
    #define SYSTEM_POSIX    1
#else
    #define SYSTEM_POSIX    0
#endif

#if defined(_WIN32) && defined (_WIN64)
    #define SYSTEM_WINDOWS  1
    #define SYSTEM_NAME     "Windows 64-bit"
#elif defined(_WIN32) && !defined(_WIN64)
    #error "Only Windows 64bit is supported"
#else
    #define SYSTEM_WINDOWS  0
#endif

#if defined(__APPLE__) && defined (__MACH__)
    #define SYSTEM_MAC      1
    #define SYSTEM_NAME     "MacOS"
#else
    #define SYSTEM_MAC      0
#endif

#if !defined(__ANDROID__) && defined (__gnu_linux__)
    #define SYSTEM_LINUX    1
    #define SYSTEM_GNULINUX 1
    #define SYSTEM_NAME     "GNU/Linux"
#else
    #define SYSTEM_LINUX    0
    #define SYSTEM_GNULINUX 0
#endif

#if !defined(SYSTEM_NAME)
    #error "System identification failed (or not supported) !"
#endif


// NOTE: This header contains type definitions

#include <stdint.h>


// Integer types
// Unsigned
typedef uint64_t    u64;
typedef uint32_t    u32;
typedef uint16_t    u16;
typedef uint8_t     u8;

#define U64_MAX UINT64_MAX
#define U32_MAX UINT32_MAX
#define U16_MAX UINT16_MAX
#define U8_MAX  UINT8_MAX

// Signed
typedef int64_t    i64;
typedef int32_t    i32;
typedef int16_t    i16;
typedef int8_t     i8;

#define I64_MAX INT64_MAX
#define I32_MAX INT32_MAX
#define I16_MAX INT16_MAX
#define I8_MAX  INT8_MAX

// Floating point
typedef float     f32;
typedef double    f64;

// Boolean
typedef int8_t    b8;
#define TRUE  1
#define FALSE 0

// Check type sizes
STATIC_ASSERT(sizeof(u64) == 8, "u64 size is wrong.");
STATIC_ASSERT(sizeof(u32) == 4, "u32 size is wrong.");
STATIC_ASSERT(sizeof(u16) == 2, "u16 size is wrong.");
STATIC_ASSERT(sizeof(u8) == 1, "u8 size is wrong.");

STATIC_ASSERT(sizeof(i64) == 8, "i64 size is wrong.");
STATIC_ASSERT(sizeof(i32) == 4, "i32 size is wrong.");
STATIC_ASSERT(sizeof(i16) == 2, "i16 size is wrong.");
STATIC_ASSERT(sizeof(i8) == 1, "i8 size is wrong.");

STATIC_ASSERT(sizeof(f32) == 4, "f32 size is wrong.");
STATIC_ASSERT(sizeof(f64) == 8, "f64 size is wrong.");

STATIC_ASSERT(sizeof(b8) == 1, "b8 size is wrong.");
STATIC_ASSERT(TRUE, "True has wrong value.");
STATIC_ASSERT(!FALSE, "False has wrong value.");


enum memory_alloc_tag
{
    // Fixed tags, they should stay here as they are necessary to the base layer
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_STRING,
    MEMORY_TAG_FIO_DATA, // Allocated data used to store File inputs temporarly in memory (read_file)

    // Free tags, those can be anything
    MEMORY_TAG_RENDERER,    // Allocated data used by the renderer for it to work
    MEMORY_TAG_RENDER_DATA, // Allocated data exploited by the renderer (Voxel data for example, textures ...)

    // Tag count
    MEMORY_TAG_COUNT,
};

__attribute__( (unused) ) static char *memory_alloc_tag_names[MEMORY_TAG_COUNT] =
{
    // Fixed tags, they should stay here as they are necessary to the base layer
    "MEMORY_UNKNOWN    ",
    "MEMORY_DARRAY     ",
    "MEMORY_STRING     ",
    "MEMORY_FIO_DATA   ",

    // Free tags, those can be anything
    "MEMORY_RENDERER   ",
    "MEMORY_RENDER_DATA",
};


#define ANSI_ESC     "\033"

#define ANSI_CLS_SCR ANSI_ESC "[2J"

#define T_CLEAR      ANSI_ESC "[0m"
#define T_BOLD       ANSI_ESC "[1m"
#define T_DIM        ANSI_ESC "[2m"
#define T_ITAL       ANSI_ESC "[3m"
#define T_UNDER      ANSI_ESC "[4m"
#define T_BLNK       ANSI_ESC "[5m"
#define T_REV        ANSI_ESC "[7m"
#define T_HIDN       ANSI_ESC "[8m"
#define T_STRK       ANSI_ESC "[9m"

// Colors
#define T_BLACK      ANSI_ESC "[30m"
#define T_RED        ANSI_ESC "[31m"
#define T_GREEN      ANSI_ESC "[32m"
#define T_YELLOW     ANSI_ESC "[33m"
#define T_BLUE       ANSI_ESC "[34m"
#define T_MAGENTA    ANSI_ESC "[35m"
#define T_CYAN       ANSI_ESC "[36m"
#define T_WHITE      ANSI_ESC "[37m"
#define T_BBLACK     ANSI_ESC "[90m"
#define T_BRED       ANSI_ESC "[91m"
#define T_BGREEN     ANSI_ESC "[92m"
#define T_BYELLOW    ANSI_ESC "[93m"
#define T_BBLUE      ANSI_ESC "[94m"
#define T_BMAGENTA   ANSI_ESC "[95m"
#define T_BCYAN      ANSI_ESC "[96m"
#define T_BWHITE     ANSI_ESC "[97m"

// Background
#define TB_BLACK     ANSI_ESC "[40m"
#define TB_RED       ANSI_ESC "[41m"
#define TB_GREEN     ANSI_ESC "[42m"
#define TB_YELLOW    ANSI_ESC "[43m"
#define TB_BLUE      ANSI_ESC "[44m"
#define TB_MAGENTA   ANSI_ESC "[45m"
#define TB_CYAN      ANSI_ESC "[46m"
#define TB_WHITE     ANSI_ESC "[47m"
#define TB_BBLACK    ANSI_ESC "[100m"
#define TB_BRED      ANSI_ESC "[101m"
#define TB_BGREEN    ANSI_ESC "[102m"
#define TB_BYELLOW   ANSI_ESC "[103m"
#define TB_BBLUE     ANSI_ESC "[104m"
#define TB_BMAGENTA  ANSI_ESC "[105m"
#define TB_BCYAN     ANSI_ESC "[106m"
#define TB_BWHITE    ANSI_ESC "[107m"
#define TB_DEFAULT   ANSI_ESC "[49m"
#define T_DEFAULT    ANSI_ESC "[39m"
#define T_COL_RES    ANSI_ESC "[0m"

#define T_RGB(r, g, b)       ANSI_ESC "[38;2;" #r ";" #g ";" #b "m"
#define TB_RGB(r, g, b)      ANSI_ESC "[48;2;" #r ";" #g ";" #b "m"


#define MK_FLAG(n)           (1 << n)
#define IS_FLAGS(d, f)       ( (d &f) == f )
#define CONTAINS_FLAGS(a, b) (a &b)
#define IS_FLAG_N(f, b)      (a & (1 << b)



// NOTE: This translation unit contains the necessary abstractions of the platform

// Memory:
void   *platform_alloc(u64    size);
void   *platform_realloc(void *ptr, u64 size);
void    platform_free(void   *ptr);
void    platform_memset(void *ptr, u8 data, u64 byte_count);
void    platform_memcpy(void *dest, void *src, u64 byte_count);
i32     platform_memcmp(void *a, void *b, u64 n);
void   *platform_memmove(void *a, void *b, u64 n);

// Returns a millisecond time, such that if it is called n milliseconds appart, the difference between the two numbers should be n
u64     platform_millis(void);


// Contains File Input/Output (hence fio)


// TODO: Make this rely on the platform layer
#include <stdio.h>

// Utils

/**
 * @brief Reads an entire file
 *
 * @param filepath The path of the file
 * @param file_size Out: the size of the file
 * @return void* A pointer to an allocated block of memory containing the file's contents, must be freed afterwise
 */
void   *fio_read_whole_file(const char *filepath, u64 *file_size);


#include <argp.h>
#include <stdarg.h>

#define ANSI_RESET T_CLEAR

enum log_levels
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
};

extern char *log_level_prefixes[LOG_LEVEL_COUNT];

extern char *log_level_colors[LOG_LEVEL_COUNT];

typedef u64 timer;

void    logging_start();
void    logging_stop();
void    logging_msg(u32 level, u32 line, char *filename, char *message, ...);

#define TRACE(msg, ...) \
        logging_msg(LOG_LEVEL_TRACE, __LINE__, __FILE__, msg, ## __VA_ARGS__)

#define DEBUG(msg, ...) \
        logging_msg(LOG_LEVEL_DEBUG, __LINE__, __FILE__, msg, ## __VA_ARGS__)

#define INFO(msg, ...) \
        logging_msg(LOG_LEVEL_INFO, __LINE__, __FILE__, msg, ## __VA_ARGS__)

#define WARN(msg, ...) \
        logging_msg(LOG_LEVEL_WARNING, __LINE__, __FILE__, msg, ## __VA_ARGS__)

#define ERROR(msg, ...) \
        logging_msg(LOG_LEVEL_ERROR, __LINE__, __FILE__, msg, ## __VA_ARGS__)

#define FATAL(msg, ...) \
        logging_msg(LOG_LEVEL_FATAL, __LINE__, __FILE__, msg, ## __VA_ARGS__)

// Assertions
#define ASSERT_MSG(exp, msg)                                                                            \
        if (exp)                                                                                            \
        {                                                                                                   \
        }                                                                                                   \
        else                                                                                                \
        {                                                                                                   \
            FATAL("Assertion (%s) failed, message='%s' on %s:%d, aborting", #exp, msg, __FILE__, __LINE__); \
            DEBUG_BRK();                                                                                    \
        }

#define ASSERT(exp)                                                                              \
        if (exp)                                                                                     \
        {                                                                                            \
        }                                                                                            \
        else                                                                                         \
        {                                                                                            \
            FATAL("Assertion (%s) failed, message='' on %s:%d, aborting", #exp, __FILE__, __LINE__); \
            DEBUG_BRK();                                                                             \
        }

#define TIMER_START() \
        platform_millis();

#define TIMER_END(t) \
        (platform_millis() - t);

#define TIMER_LOG(t, n)                                                               \
        {                                                                                 \
            timer time = TIMER_END(t);                                                    \
            INFO( " [TIMER] '" n "' : %.1f s - (%lu ms)", (f32)time / (f32)1.0e3, time ); \
        }


#define MAX(a, b)        ( (a) < (b) ? b : a )
#define MIN(a, b)        ( (a) < (b) ? a : b )

// Clamps x under a
#define CLAMP_UP(x, a)   ( MIN(x, a) )

// Calmps x over a
#define CLAMP_DOWN(x, a) ( MAX(x, a) )

// Clamps x between a and b
#define CLAMP(x, a, b)   ( CLAMP_DOWN(CLAMP_UP(x, b), a) )


// NOTE: Contains memory allocators for the engine


// Units
#define MEMORY_UNIT_BYTE        1
// NOTE: Can be changed to use KB/MB/GB (1000) or KiB/MiB/GiB (1024)
#define MEMORY_UNIT_KILO        1024
#define MEMORY_UNIT_MEGA        (MEMORY_UNIT_KILO * MEMORY_UNIT_KILO)
#define MEMORY_UNIT_GIGA        (MEMORY_UNIT_MEGA * MEMORY_UNIT_KILO)

#define MEMORY_UNIT_BYTE_NAME   "B"
#define MEMORY_UNIT_KILO_NAME   "KiB"
#define MEMORY_UNIT_MEGA_NAME   "MiB"
#define MEMORY_UNIT_GIGA_NAME   "GiB"


#define MEMORY_ALLOC_TOTAL_NAME "MEMORY_TOTAL      "

struct mem_usage
{
    u64    total_memory_usage;
    u64    tags_memory_usage[MEMORY_TAG_COUNT];
};

// This struct is written at the beginning of every allocated memory chunk
struct mem_chunk_header
{
    // The number of bytes that are allocated
    u64    allocated_size;

    // The type of memory that is allocated
    u32    allocated_type;
};

__attribute__( (unused) ) static struct mem_usage current_memory_usage;

/**
 * @brief Start the memory subsystem
 *
 */
void    memory_start();

/**
 * @brief Shut the memory subsystem down
 *
 */
void    memory_shutdown();

/**
 * @brief Allocates a block of memory
 *
 * @param size The size of the block
 * @param tag The tag of the block
 * @return void* A pointer to the beginning of the block
 */
void   *mem_allocate(u64 size, enum memory_alloc_tag tag);

/**
 * @brief Reallocates a block of memory, copying old contents
 *
 * @param ptr The old block
 * @param size The size of the new block
 * @return void* A pointer to the beginning of the new block
 */
void   *mem_reallocate(void *ptr, u64 size);

/**
 * @brief Frees a block of memory
 *
 * @param ptr A pointer to the beginning of the block
 */
void    mem_free(void   *ptr);

/**
 * @brief Set an entire block of memory
 *
 * @param ptr A pointer to the beginning of the block to set
 * @param data The byte to set the block with
 * @param byte_count The number of bytes to set
 */
void    mem_memset(void *ptr, u8 data, u64 byte_count);

/**
 * @brief Copies a block of memory
 *
 * @param dest The destination
 * @param src The source
 * @param byte_count The number of bytes to copy
 * @attention The regions may not overlapp
 */
void    mem_memcpy(void *dest, void *src, u64 byte_count);

/**
 * @brief Compares two bloks of memory
 *
 * @param a The first block of memory
 * @param b The second block of memory
 * @param byte_count The number of bytes to compare
 * @returns 0 if equal, non-zero if different
 */
i32     mem_memcmp(void *a, void *b, u64 byte_count);

/**
 * @brief Moves a block of memory
 *
 * @param a The first block of memory
 * @param b The second block of memory
 * @param byte_count The number of bytes to move
 * @returns a
 */
void   *mem_memmove(void *a, void *b, u64 byte_count);

/**
 * @brief Prints the memory using accoring to each tag
 *
 */
void    mem_print_memory_usage();

/**
 * @brief Converts a `u64` size into a human readable format (3.44KiB, 4.25GiB ...)
 *
 */
u64     mem_size_get_pretty_string(u64 size, char *buffer);


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

/**
 * @file darray.h
 * @brief This file along with darray.c contains the code for a generic, dynamic array implementation
 */

#ifndef __BASE_DARRAY__
#define __BASE_DARRAY__

enum darray_fields
{
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_TAG,
    DARRAY_FIELD_LENGTH
};

/*
 * Represents a comparison between two arbitrary objects.
 * result r is :
 *  - r < 0 if a < b
 *  - r > 0 if a > b
 *  - r = 0 if a = b
 */
typedef i32 (*darray_usr_compare_func)(void *a, void *b);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR    2
#define DARRAY_MAGIC_NUMBER     0xFEEDBEEF

#define darray(type) \
        type *

// Private functions
// NOTE: Those should not be called directly outside the darray implementation
void   *_darray_create(u64 capacity, u64 stride, enum memory_alloc_tag);
void    _darray_destroy(void   *ptr);

void   *_darray_resize(void *array, u64 new_size);

void   *_darray_push(void *array, void *data);
void   *_darray_pop(void *array, void *dest);

void   *_darray_insert_at(void *array, u64 index, void *obj);
void   *_darray_pop_at(void *array, u64 index, void *dest);

u64     _darray_get_field(void *array, u64 field);
void    _darray_set_field(void *array, u64 field, u64 value);

b8      _darray_is_sorted(void *array, darray_usr_compare_func cmp);
b8      _darray_is_strictly_sorted(void *array, darray_usr_compare_func cmp);
b8      _darray_qsort(void *array, darray_usr_compare_func cmp);

/**
 * @brief Creates a dynamic array, with the typename provided.
 *
 * @param type The typename of the stored object
 */
#define darray_create(type) \
        _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), MEMORY_TAG_DARRAY)

/**
 * @brief Creates a dynamic array, with the size provided.
 *
 * @param type The size of the stored object
 */
#define darray_create_sized(size) \
        _darray_create(DARRAY_DEFAULT_CAPACITY, size, MEMORY_TAG_DARRAY)

/**
 * @brief Creates a dynamic array, with the typename and memory tag provided.
 *
 * @param type The typename of the stored object
 * @param tag The memory tag to use
 */
#define darray_create_tag(type, tag) \
        _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), tag)

/**
 * @brief Creates a dynamic array, with the typename and the capacity provided.
 *
 * @param type The typename of the stored object
 * @param capacity The capacity to reserve
 */
#define darray_reserve(type, capacity) \
        _darray_create( capacity, sizeof(type) )

/**
 * @brief Destroys an dynamic array
 *
 * @param array The array
 */
#define darray_destroy(array) \
        _darray_destroy(array)

/**
 * @brief Gets the number of elements in the array
 *
 * @param array The array
 */
#define darray_length(array) \
        _darray_get_field(array, DARRAY_LENGTH)

/**
 * @brief Gets the current capacity of the array ()
 *
 * @param array The array
 */
#define darray_capacity(array) \
        _darray_get_field(array, DARRAY_CAPACITY)

/**
 * @brief Gets the stride of the array (the size of a stored object) in bytes
 *
 * @param array The array
 */
#define darray_stride(array) \
        _darray_get_field(array, DARRAY_STRIDE)

/**
 * @brief Gets the memory tag used for allocations
 *
 * @param array The array
 */
#define darray_tag(array) \
        _darray_get_field(array, DARRAY_TAG)

/**
 * @brief Pushes an object in the array
 *
 * @param array The array
 * @param obj The object to copy and push into the array
 * @attention This will not work with string literals
 * @attention This macro-function may modify the array pointer.
 */
#define darray_push(array, obj)             \
        {                                       \
            typeof(obj) temp = obj;             \
            array = _darray_push(array, &temp); \
        }

/**
 * @brief Pushes an object pointed to in the array
 *
 * @param array The array
 * @param ptr A pointer from which to copy from the data to store
 * @attention This will not work with string literals
 * @attention This macro-function may modify the array pointer.
 */
#define darray_push_ptr(array, ptr)       \
        {                                     \
            array = _darray_push(array, ptr); \
        }

/**
 * @brief Pops an object: removes and returns the last object of the array
 *
 * @param array The array
 * @attention This macro-function may modify the array pointer.
 */
#define darray_pop(array, dest) \
        array = _darray_pop(array, dest);

/**
 * @brief Inserts an object in the array at a specific index
 *
 * @param array The array
 * @param obj The object to insert
 * @param index The index of the added object
 *
 * @note It is garanteed that doing `array[index]` returns the same object inserted with this function
 * @attention This will not work with string literals
 * @attention This macro-function may modify the array pointer.
 */
#define darray_insert_at(array, obj, index)             \
        {                                                   \
            typeof(obj) temp = obj;                         \
            array = _darray_insert_at(array, index, &temp); \
        }

/**
 * @brief Pops an object from an index: removes and returns an object at a specific index
 *
 * @param array The array
 * @param index The index of the object to remove
 * @param dest The destination of the removed object
 *
 * @note It is garanteed that doing `array[index]` returns the object that was after the removed object.
 * @attention This macro-function may modify the array pointer.
 */
#define darray_pop_at(array, index, dest) \
        array = _darray_pop_at(array, index, dest);

/**
 * @brief Sorts the array, based on a user provided comparison function (order on a set)
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note This sort uses the quick sort algorithm. Worst cas is O(n^2). Best case/average is O(n log n).
 * @note It must be guaranteed that darray_sorted returns true after this call. The list may not necessarly strictly sorted.
 * @attention This function modifies memory, but does not require any reallocation/pointer modification.
 */
#define darray_qsort(array, cmp) \
        _darray_qsort(array, (darray_usr_compare_func)cmp);

/**
 * @brief Checks wether or not the array is sorted, using a totally ordered set relation, user provided.
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note Two elements a,b are considered as sorted if cmp(a, b) = 0.
 */
#define darray_sorted(array, cmp) \
        _darray_is_sorted(array, (darray_usr_compare_func)cmp)

/**
 * @brief Checks wether or not the array is strictly sorted, using a totally ordered set relation, user provided.
 *
 * @param array The array
 * @param cmp The comparison function
 *
 * @note Two elements a,b are not considered as sorted if cmp(a, b) = 0.
 */
#define darray_strict_sorted(array, cmp) \
        _darray_is_strictly_sorted(array, (darray_usr_compare_func)cmp)

#endif //__BASE_DARRAY__

/**
 * @file      hashmap.h
 * @brief     Header of a simple hashmap implementation
 * @date      Sat May 20 19:36:17 2023
 * @author    albin
 * @copyright BSD-3-Clause
 *
 * This module
 */


#include <alloca.h>

typedef u64 (*hashmap_hash_func)(void *obj, u64 size);

typedef struct
{
    u32                  bucket_count;
    void               **buckets; // darrays
    hashmap_hash_func    hash_func;

    u64                  obj_stride;
    u64                  key_stride;

    u64                  count;
} hashmap;

void    hashmap_create(hashmap *hm, u32 bucket_count, u64 key_stride, u64 obj_stride,
                       hashmap_hash_func hash_func);

void    hashmap_destroy(hashmap   *hm);

void    hashmap_insert(hashmap *hm, void *key, void *object);
void   *hashmap_lookup(hashmap *hm, void *key);
b8      hashmap_remove(hashmap *hm, void *key);

#define hashmap_count(hm) \
        ( (hm)->count )


/**
 * @file str.h
 * @author Albin Chaboissier (chalbin73@gmail.com)
 * @brief A simple custom string data structure
 * @version 0.1
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023
 *
 */


typedef struct
{
    u64    str_length;
    u8    *chars;
} str;

void    str_create(str *string, u8 *data, u64 data_length);

#define str_from_literal(str, lit) \
        str_create(str, lit, sizeof(lit) - 1);

void    str_free(str   *string);


/**
 * @file tight_b8.h
 * @author Albin Chaboisser (chalbin73@gmail.com)
 * @brief A tight boolean implementation (allows to store 8 booleans par bytes)
 * @version 0.1
 * @date 2023-05-06
 *
 * @copyright Copyright (c) 2023
 *
 */


#define TB8_MOD8(x)     ( (x) & 7 )
#define TB8_U64_DIV8(x) ( (x) >> 3 )

/**
 * @brief Sets all the boolean to false in a tight b8
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE
 */
b8      tb8_set_all_false(b8 *array, u64 bool_count);

/**
 * @brief Sets all the boolean to true in a tight b8
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE
 */
b8      tb8_set_all_true(b8 *array, u64 bool_count);

/**
 * @brief Sets a boolean in a tight b7
 *
 * @param array The tight boolean array
 * @param index The index of the boolean
 * @param value The value to set the index to
 * @return b8 TRUE
 */
b8      tb8_set(b8 *array, u64 index, b8 value);

/**
 * @brief Toggles a boolean in the array
 *
 * @param array The tight boolean array
 * @param index The index to toggle
 * @return b8 The value of the boolean before the toggle
 */
b8      tb8_toggle(b8 *array, u64 index);

/**
 * @brief Gets a boolean value in the array
 *
 * @param array The tight boolean array
 * @param index The index to get
 * @return b8 The value at index
 */
b8      tb8_get(b8 *array, u64 index);

/**
 * @brief Wether or not the array contains a true
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE or FALSE
 */
b8      tb8_some_true(b8 *array, u64 bool_count);

/**
 * @brief Wether or not all the boolean are false
 *
 * @param array The tight boolean array
 * @param bool_count The number of booleans
 * @return b8 TRUE or FALSe
 */
b8      tb8_all_true(b8 *array, u64 bool_count);


// Simple allocator that allocates/deallocates chunks at a time


typedef struct mem_pool_empy_chunk_header
{
    struct mem_pool_empy_chunk_header   *next;
} mem_pool_empy_chunk_header;

/**
 * @brief A memory pool allocator object
 *
 */
typedef struct
{
    void                         *memory;
    u64                           chunk_size;
    u64                           chunk_count;

    u64                           free_chunk_count;

    // Linked list of pointers of empty chunks
    mem_pool_empy_chunk_header   *start;
} mem_pool;

/**
 * @brief Creates a memory pool with a memory block, cut into `chunk_count` chunks of `chunk_size`
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param chunk_size The size of a chunk
 * @param chunk_count The number of chunk to provide
 */
void    mem_pool_create(mem_pool *pool, void *memory, u64 chunk_size, u64 chunk_count);

/**
 * @brief Creates a memory pool with a memory block, of a at least `memory_size` bytes, and creates biggest chunks possible
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param memory_size The size of the memory block
 * @param chunk_count The number of chunk to provide
 */
void    mem_pool_create_sized_count(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_count);

/**
 * @brief Creates a memory pool with a memory block, of a at least `memory_size` bytes, and creates most chunks of `chunk_size` possible
 *
 * @param pool A zero-out pool object to create the pool in
 * @param memory The memory block
 * @param memory_size The size of the memory block
 * @param chunk_count The size of the chunks
 */
void    mem_pool_create_sized_size(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_size);

/**
 * @brief Allocates a chunk in the pool
 *
 * @param pool The pool object
 * @return void* A pointer to the allocated chunk, is NULL if no more chunk are available
 */
void   *mem_pool_alloc(mem_pool   *pool);

/**
 * @brief Frees a chunk in the pool
 *
 * @param pool The pool object
 * @param ptr A pointer to the chunk to be deallocated
 */
void    mem_pool_dealloc(mem_pool *pool, void *ptr);

/**
 * @brief Destroys the pool
 *
 * @param pool The pool to destroy
 * @return void* A pointer to the block of memory
 */
void   *mem_pool_destroy(mem_pool   *pool);

/**
 * @brief Get a list of pointer to all allocated block
 *
 * @param pool The pool object
 * @param block_list The list of pointer, can be NULL to query count
 * @param count The number of allocated block
 * @deprecated This function is deprectated
 */
void    mem_pool_get_alloced_blocks(mem_pool *pool, void **block_list, u32 *count);

/**
 * @brief Gets a pointer to a block via its index
 *
 * @param pool The pool object
 * @param index The block's index
 */
void   *mem_pool_index(mem_pool *pool, u64 index);

/**
 * @brief Provides a basic handle manager, for api design. This handle manger handles creation, destruction, memory managment of objects
            hidden behind an opaque handle type.
            The API backend using this handle manager, may use the fucntion interface, and the handle type, in order to operate on the underlying
            objets, and may pass the opaque handles to the user (on the frontend).
            This way, the user, only works with opaque handles, and it secures the use of the api.
            The handle function interface, should not be used bu the user.
 *
 */

/**
 * @brief A handle, this type should preferably not be given, or used, on the frontend of an api, so that handles "types" can be recognised by the user */
typedef u32 handle;
#define NULL_HANDLE 0

typedef struct
{
    u16    unique_counter;
    b8     free;
} handle_mgr_block_header;

/**
 * @brief The actuak handle manager object, functions to operate on this are prefixed with handle_mgr
 *
 */
typedef struct
{
    void   *memory;
    u64     memory_size;

    u64     managed_size;
    u64     block_size;
    u64     alloced_blocks;
    u64     block_count;

    void   *first_free_block;
} handle_mgr;

/**
 * @brief Returns the size of memory to be allocated for use with the handle manager for it to be able to allocate `concurrent_alloc_count` concurrently
 *
 * @param concurrent_alloc_count The number of allocations
 * @param managed_size The size of the object to be managed
 * @return u64 The size of the mermory
 */
u64       handle_mgr_get_size_for_count(u64 concurrent_alloc_count, u64 managed_size);

/**
 * @brief Create the handle memory
 *
 * @param mgr A pointer to a handle manager
 * @param memory Memory that the handle manager manages
 * @param memory_size The size of the memory that the manager will be able to access
 * @param managed_size The size of the object managed by this instance
 * @return b8 Success ?
 */
b8        handle_mgr_create(handle_mgr *mgr, void *memory, u64 memory_size, u64 managed_size);

/**
 * @brief Allocates a new object
 *
 * @param mgr A pointer to a handle manager
 * @return handle A handle to the new free object
 */
handle    handle_mgr_allocate(handle_mgr   *mgr);

/**
 * @brief Frees an object
 *
 * @param mgr A pointer to a handle manager
 * @param hndl The handle of the object ti be freed
 * @return b8 Was a free operation made
 */
b8        handle_mgr_free(handle_mgr *mgr, handle hndl);

/**
 * @brief Dereferences an handle
 *
 * @param mgr A pointer to a handle manager
 * @param hndl The handle to dereference
 * @return void* A pointer to the dereferenced object
 */
void     *handle_mgr_deref(handle_mgr *mgr, handle hndl);

/**
 * @brief Retrieves number of allocated handles
 *
 * @param mgr A pointer to a handle manager
 * @return u64 The number of allocated handles
 */
u64       handle_mgr_get_count(handle_mgr   *mgr);

/**
 * @brief A pointer to a handle manager
 *
 * @param mgr Destroys a handle manager
 * @return void* The memory that was provided to the manager
 */
void     *handle_mgr_destroy(handle_mgr   *mgr);

// Beginning of implementation
// (guard_open.h) please do not include this file directly
#ifdef BASE_IMPLEMENTATION

#ifndef BASE_IMPL_GUARD
#define BASE_IMPL_GUARD

/* ----------------- IMPLEMENTATION --------------- */

#if SYSTEM_GNULINUX
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
// TODO: Use custom allocator
void *
    platform_alloc(u64    size)
{
    return malloc(size);
}

void *
    platform_realloc(void *ptr, u64 size)
{
    return realloc(ptr, size);
}

void
    platform_free(void   *ptr)
{
    free(ptr);
}

void
    platform_memset(void *ptr, u8 data, u64 byte_count)
{
    memset(ptr, data, byte_count);
}

void
    platform_memcpy(void *dest, void *src, u64 byte_count)
{
    memcpy(dest, src, byte_count);
}

i32
    platform_memcmp(void *a, void *b, u64 n)
{
    return memcmp(a, b, n);
}

void *
    platform_memmove(void *a, void *b, u64 n)
{
    return memmove(a, b, n);
}

u64
    platform_millis(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    u64 millis = (time.tv_sec * 1e3) + (time.tv_nsec / 1e6);
    return millis;
}

#endif


char *log_level_prefixes[LOG_LEVEL_COUNT] =
{
    "[TRACE]",
    "[DEBUG]",
    "[INFO ]",
    "[WARN ]",
    "[ERROR]",
    "[FATAL]",
};

char *log_level_colors[LOG_LEVEL_COUNT] =
{
    ANSI_ESC "[0m",       // White
    ANSI_ESC "[32m",      // Green
    ANSI_ESC "[36m",      // Cyan
    ANSI_ESC "[33m",      // Yellow
    ANSI_ESC "[31m",      // Red
    ANSI_ESC "[31;1m"     // Red, bold
};

void
    logging_start()
{
    // TODO: Output to file
}

void
    logging_stop()
{
    // TODO: Output to file
}

void
    logging_msg(u32 level, u32 line, char *filename, char *message, ...)
{
    // TODO: Display filename + line on option

    printf("%s%s: ", log_level_colors[level], log_level_prefixes[level]);

    va_list arg_list;
    va_start(arg_list, message);
    vprintf(message, arg_list);
    va_end(arg_list);

    printf("%s\n", ANSI_RESET);
}

void *
_darray_create(u64 capacity, u64 stride, enum memory_alloc_tag tag)
{
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 array_size  = capacity * stride;
    u64 *new_array  = mem_allocate(array_size + header_size, tag);
    mem_memset(new_array, 0, array_size + header_size);
    new_array[DARRAY_CAPACITY] = capacity;
    new_array[DARRAY_LENGTH]   = 0;
    new_array[DARRAY_STRIDE]   = stride;
    new_array[DARRAY_TAG]      = tag;
    return (void *)(new_array + DARRAY_FIELD_LENGTH);
}

void
_darray_destroy(void   *ptr)
{
    if (ptr)
    {
        u64 *header = (u64 *)ptr - DARRAY_FIELD_LENGTH;
        header[DARRAY_CAPACITY] = 0;
        header[DARRAY_STRIDE]   = 0;
        mem_free(header);
    }
}

u64
    _darray_get_field(void *array, u64 field)
{
    u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
    return header[field];
}

void *
    _darray_resize(void *array, u64 new_size)
{
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 tag    = darray_tag(array);

    void *temp = _darray_create(new_size, stride, tag);
    mem_memcpy(temp, array, length * stride);

    _darray_set_field(array, DARRAY_LENGTH, length);
    _darray_destroy(array);
    return temp;
}

void
    _darray_set_field(void *array, u64 field, u64 value)
{
    u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}

void *
    _darray_push(void *array, void *data)
{
    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    if ( length >= darray_capacity(array) )
    {
        array = _darray_resize(array, capacity * DARRAY_RESIZE_FACTOR);
    }

    u64 addr = (u64)array;
    addr += (length * stride);
    mem_memcpy( (void *)addr, data, stride );
    _darray_set_field(array, DARRAY_LENGTH, length + 1);
    return array;
}

void *
_darray_pop(void *array, void *dest)
{
    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    u64 addr = (u64)array;
    addr += (length - 1) * stride;
    mem_memcpy(dest, (void *)addr, stride);
    _darray_set_field(array, DARRAY_LENGTH, length - 1);
    (void)capacity; // TODO: Allow darray to shrink<
    return array;
}

void *
_darray_pop_at(void *array, u64 index, void *dest)
{
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);

    if (index >= length)
    {
        ERROR("Index outside of bound of darray, length=%d index=%d", length, index);
        return array;
    }

    u64 addr = (u64)array;
    if (dest != NULL)
    {
        mem_memcpy(dest, (void *)( addr + (index * stride) ), stride);
    }

    if ( index != (length - 1) )
    {
        mem_memmove(
            (void *)( addr + (index * stride) ),
            (void *)( addr + ( (index + 1) * stride ) ),
            stride * ( (length - index) - 1 )
            );
    }

    _darray_set_field(array, DARRAY_LENGTH, length - 1);
    return array;
}

void *
_darray_insert_at(void *array, u64 index, void *obj)
{

    u64 length   = darray_length(array);
    u64 stride   = darray_stride(array);
    u64 capacity = darray_capacity(array);

    if (index >= length)
    {
        ERROR("Index outside of bound of darray, length=%d index=%d", length, index);
        return array;
    }

    if ( length >= darray_capacity(array) )
    {
        array = _darray_resize(array, capacity * DARRAY_RESIZE_FACTOR);
    }

    u64 addr = (u64)array;

    if (index != length - 1)
    {
        mem_memmove(
            (void *)( addr + ( (index + 1) * stride ) ),
            (void *)( addr + (index * stride) ),
            stride * (length - index)
            );
    }

    mem_memcpy( (void *)( addr + (index * stride) ), obj, stride );

    _darray_set_field(array, DARRAY_LENGTH, length + 1);
    return array;
}

// Sorting functions
b8
_darray_is_sorted(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    void *prev = array;
    for(u64 i = 1; i < length; i++)
    {
        void *new = (void *)( (u64)prev + stride );

        if(cmp(prev, new) > 0)
        {
            return FALSE;
        }
        prev = new;
    }

    return TRUE;
}

b8
_darray_is_strictly_sorted(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    void *prev = array;
    for(u64 i = 1; i < length; i++)
    {
        void *new = (void *)( (u64)prev + stride );

        if(cmp(prev, new) >= 0)
        {
            return FALSE;
        }
        prev = new;
    }

    return TRUE;
}

// Convenience function, to do the necessary casts
#define _DARRAY_INDEX(array, stride, i) \
        ( (void *)( (u64)(array) + ( (i) * (stride) ) ) )

void
_darray_qsort_vswap(void *a, void *b, u64 byte_count)
{
    // Compilers should be able to optimize this code quite well
    u8 *p;
    u8 *q;
    u8 *end = (u8 *)( (u64)a + byte_count );

    u8 t;
    for(p = a, q = b; p < end; p++, q++)
    {
        t  = *p;
        *p = *q;
        *q = t;
    }
}

#include <alloca.h>
#define _DARRAY_SWAP(array, stride, a, b)               \
        if(a != b) \
        {                                               \
            void *a_p = _DARRAY_INDEX(array, stride, a); \
            void *b_p = _DARRAY_INDEX(array, stride, b); \
            _darray_qsort_vswap(a_p, b_p, stride);   \
        }

// Quick sort implementation
u64
_darray_qsort_partition(void *array, u64 stride, u64 start, u64 end, darray_usr_compare_func cmp)
{
    // Chose pivot
    u64 pivot_index = ( (end - 1L) + start ) / 2L;
    //u64 pivot_index = end - 1L;

    // Move pivot to end
    _DARRAY_SWAP(array, stride, pivot_index, end - 1L);
    void *pivot = _DARRAY_INDEX(array, stride, end - 1L);

    // Start of elements greater than pivot
    u64 i_sup = start;

    for(u64 i = start; i < end - 1L; i++)
    {
        // Compare current and pivot
        void *current = _DARRAY_INDEX(array, stride, i);
        i32 cmp_r     = cmp(current, pivot);

        if(cmp_r < 0) // current smaller that pivot
        {
            _DARRAY_SWAP(array, stride, i_sup, i);
            i_sup++;
        }
        else // current bigger that pivot
        {
            // NO-OP
        }
    }

    // Partition finished. Move pivot in between two zones
    _DARRAY_SWAP(array, stride, i_sup, end - 1L);

    // Return new pivot position
    return i_sup;
}

void
_darray_qsort_rec(void *array, u64 stride, u64 start, u64 end, darray_usr_compare_func cmp)
{
    if(start < end )
    {
        //printf("%ud -- %ud\n", start, end);
        u64 pivot_index = _darray_qsort_partition(array, stride, start, end, cmp);

        // Quick sort the two partitioned areas
        _darray_qsort_rec(array, stride, start, pivot_index, cmp);
        _darray_qsort_rec(array, stride, pivot_index + 1L, end, cmp);
    }
}

b8
_darray_qsort(void *array, darray_usr_compare_func cmp)
{
    u64 length = _darray_get_field(array, DARRAY_LENGTH);
    u64 stride = _darray_get_field(array, DARRAY_STRIDE);

    _darray_qsort_rec(array, stride, 0L, length, cmp);
    return TRUE;
}

#undef _DARRAY_INDEX
#undef _DARRAY_SWAP


void
hashmap_create(hashmap *hm, u32 bucket_count, u64 key_stride, u64 obj_stride,
               hashmap_hash_func hash_func)

{
    hm->bucket_count = bucket_count;
    hm->key_stride   = key_stride;
    hm->obj_stride   = obj_stride;
    hm->hash_func    = hash_func;
    hm->count        = 0;

    hm->buckets = mem_allocate(sizeof(void *) * bucket_count, MEMORY_TAG_RENDERER);
    for (int i = 0; i < bucket_count; i++)
    {
        hm->buckets[i] = darray_create_sized(key_stride + obj_stride);
    }
}

void
    hashmap_destroy(hashmap   *hm)
{
    for (int i = 0; i < hm->bucket_count; i++)
    {
        darray_destroy(hm->buckets[i]);
    }
    mem_free(hm->buckets);
}

void
    hashmap_insert(hashmap *hm, void *key, void *object)
{
    u64 bidx = hm->hash_func(key, hm->key_stride) % hm->bucket_count;

    void *dat = alloca(hm->key_stride + hm->obj_stride);
    mem_memcpy(dat, key, hm->key_stride);
    mem_memcpy( (void *)( (u64)dat + hm->key_stride ), object, hm->obj_stride );

    // TODO: Handle already inserted objects
    darray_push_ptr(hm->buckets[bidx], dat);
    hm->count++;
}

void *
hashmap_lookup(hashmap *hm, void *key)
{
    u64 bidx   = hm->hash_func(key, hm->key_stride) % hm->bucket_count;
    void *buck = hm->buckets[bidx];
    void *elem = buck;

    u32 count = darray_length(buck);
    for (int i = 0; i < count; i++)
    {
        if (mem_memcmp(elem, key, hm->key_stride) == 0)
        {
            return (void *)( (u64)elem + hm->key_stride );
        }
        elem = (void *)( (u64)elem + (hm->key_stride + hm->obj_stride) );
    }

    // Not found
    return NULL;
}

b8
hashmap_remove(hashmap *hm, void *key)
{
    u64 bidx   = hm->hash_func(key, hm->key_stride) % hm->bucket_count;
    void *buck = hm->buckets[bidx];
    void *elem = buck;

    b8 removed = FALSE;
    for (int i = 0; i < darray_length(buck); i++)
    {
        if (mem_memcmp(elem, key, hm->key_stride) == 0)
        {
            darray_pop_at(buck, i, NULL);
            removed = TRUE;
            hm->count--;
            return TRUE;
        }
        elem = (void *)( (u64)elem + hm->key_stride + hm->obj_stride );
    }

    return removed;
}


b8
    tb8_set_all_false(b8 *array, u64 bool_count)
{
    mem_memset( array, 0x00, ( TB8_U64_DIV8(bool_count) ) + (TB8_MOD8(bool_count) == 0 ? 0 : 1) );
    return TRUE;
}

b8
    tb8_set_all_true(b8 *array, u64 bool_count)
{
    mem_memset( array, 0xFF, ( TB8_U64_DIV8(bool_count) ) + (TB8_MOD8(bool_count) == 0 ? 0 : 1) );
    return TRUE;
}

b8
    tb8_set(b8 *array, u64 index, b8 value)
{
    // Get the byte
    u8 *byte = (u8 *)&array[TB8_U64_DIV8(index)];

    // Set the value within the byte
    if (value)
    {
        *byte |= 1 << ( TB8_MOD8(index) );
    }
    else
    {
        *byte &= ~( 1 << ( TB8_MOD8(index) ) );
    }
    return TRUE;
}

b8
    tb8_toggle(b8 *array, u64 index)
{
    b8 val = tb8_get(array, index);
    tb8_set(array, index, !val);
    return val;
}

b8
    tb8_get(b8 *array, u64 index)
{
    // Get the byte
    u8 *byte = (u8 *)&array[TB8_U64_DIV8(index)];

    // Get the value within the byte

    return ( ( *byte >> ( TB8_MOD8(index) ) ) & 1 ) != 0;
}

b8
tb8_some_true(b8 *array, u64 bool_count)
{
    u64 byte_count = ( TB8_U64_DIV8(bool_count) ) + (TB8_MOD8(bool_count) == 0 ? 0 : 1);

    // Browse every byte
    for (int i = 0; i < byte_count; i++)
    {
        if (array[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

b8
tb8_all_true(b8 *array, u64 bool_count)
{
    u64 byte_count = ( TB8_U64_DIV8(bool_count) ) + (TB8_MOD8(bool_count) == 0 ? 0 : 1);

    // Browse every byte
    for (int i = 0; i < byte_count; i++)
    {
        if ( array[i] != ( (b8)0xFF ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}



void
    memory_start()
{
    TRACE("Starting memory subsystem.");
    current_memory_usage = (struct mem_usage)
    {
        0
    };
}

void
    memory_shutdown()
{
    for (int i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        if (current_memory_usage.tags_memory_usage[i] != 0)
        {
            char buf[64];

            mem_size_get_pretty_string(current_memory_usage.tags_memory_usage[i], buf);
            WARN("Memory leak detected : %s : %s not freed.", memory_alloc_tag_names[i], buf);
        }
    }

    if (current_memory_usage.total_memory_usage != 0)
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.total_memory_usage, buf);
        WARN("Total memory leak    : %s : %s not freed.", MEMORY_ALLOC_TOTAL_NAME, buf);
    }

    TRACE("Shutting memory subsystem down.");
    current_memory_usage = (struct mem_usage)
    {
        0
    };
}

void *
mem_allocate(u64 size, enum memory_alloc_tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN)
    {
        char buf[64];
        mem_size_get_pretty_string(size, buf);
        WARN("Allocating %s using MEMORY_TAG_UNKNOWN.", buf);
    }

    u64 full_alloc_size = sizeof(struct mem_chunk_header) + size;
    void *mem           = platform_alloc(full_alloc_size);
    ASSERT_MSG(mem, "Memory allocation failed.");

    current_memory_usage.total_memory_usage     += full_alloc_size;
    current_memory_usage.tags_memory_usage[tag] += full_alloc_size;

    // Write data in the header
    struct mem_chunk_header *header = mem;
    header->allocated_size = full_alloc_size;
    header->allocated_type = tag;

    void *result = mem + sizeof(struct mem_chunk_header);
    return result;
}

void *
mem_reallocate(void *ptr, u64 size)
{
    struct mem_chunk_header *full_block = ptr - sizeof(struct mem_chunk_header);
    current_memory_usage.total_memory_usage                            -= full_block->allocated_size;
    current_memory_usage.tags_memory_usage[full_block->allocated_type] -= full_block->allocated_size;

    current_memory_usage.total_memory_usage                            += size;
    current_memory_usage.tags_memory_usage[full_block->allocated_type] += size;

    full_block = platform_realloc( full_block, size + sizeof(struct mem_chunk_header) );

    void *result = (void *)( (u64)full_block + sizeof(struct mem_chunk_header) );
    return result;
}

void
mem_free(void   *ptr)
{
    ASSERT_MSG(ptr, "Invalid pointer provided.");

    // Get pointer from the beginning with the header
    void *full_chunk = ptr - sizeof(struct mem_chunk_header);
    (void)full_chunk;

    struct mem_chunk_header *header = full_chunk;
    current_memory_usage.total_memory_usage                        -= header->allocated_size;
    current_memory_usage.tags_memory_usage[header->allocated_type] -= header->allocated_size;

    platform_free(full_chunk);
}

void
    mem_memset(void *ptr, u8 data, u64 byte_count)
{
    platform_memset(ptr, data, byte_count);
}

void
    mem_memcpy(void *dest, void *src, u64 byte_count)
{
    platform_memcpy(dest, src, byte_count);
}

i32
    mem_memcmp(void *a, void *b, u64 byte_count)
{
    return platform_memcmp(a, b, byte_count);
}

void *
    mem_memmove(void *a, void *b, u64 byte_count)
{
    return platform_memmove(a, b, byte_count);
}

void
    mem_print_memory_usage()
{
    INFO("~~~~~~~~~~ Current memory usage ~~~~~~~~~~");
    INFO(
        "Memory type              |"
        " Memory usage"
        );

    // Printing total memory usage
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.total_memory_usage, buf);

        INFO("  %s : %s", MEMORY_ALLOC_TOTAL_NAME, buf);
    }

    for (int i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        char buf[64];
        mem_size_get_pretty_string(current_memory_usage.tags_memory_usage[i], buf);

        INFO("  %s : %s", memory_alloc_tag_names[i], buf);
    }
    INFO("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

u64
mem_size_get_pretty_string(u64 size, char *buffer)
{
    float val   = (float)size;
    u64 printed = 0;

    if (val > MEMORY_UNIT_GIGA)
    {
        val     = val / MEMORY_UNIT_GIGA;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_GIGA_NAME);
    }
    else if (val > MEMORY_UNIT_MEGA)
    {
        val     = val / MEMORY_UNIT_MEGA;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_MEGA_NAME);
    }
    else if (val > MEMORY_UNIT_KILO)
    {
        val     = val / MEMORY_UNIT_KILO;
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_KILO_NAME);
    }
    else
    {
        printed = sprintf(buffer, "%.2f %s", val, MEMORY_UNIT_BYTE_NAME);
    }

    return printed;
}


void
mem_pool_create(mem_pool *pool, void *memory, u64 chunk_size, u64 chunk_count)
{
    pool->memory           = memory;
    pool->chunk_count      = chunk_count;
    pool->chunk_size       = chunk_size;
    pool->free_chunk_count = chunk_count;
    pool->start            = memory;

    mem_memset(pool->memory, 0x00, chunk_count * chunk_size);

    // pool_empy_chunk_header_t *prev = memory;
    // Write empty header
    for (int i = 0; i < pool->chunk_count; i++)
    {
        void *ptr = &( (u8 *)pool->memory )[i * pool->chunk_size];

        mem_pool_empy_chunk_header *node = (mem_pool_empy_chunk_header *)ptr;
        // Push free node onto thte free list
        node->next  = pool->start;
        pool->start = node;
    }
}

void
    mem_pool_create_sized_count(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_count)
{
    u64 chunk_size = memory_size / chunk_count; // Euclidean quotient
    mem_pool_create(pool, memory, chunk_size, chunk_count);
}

void
    mem_pool_create_sized_size(mem_pool *pool, void *memory, u64 memory_size, u64 chunk_size)
{
    u64 chunk_count = memory_size / chunk_size; // Euclidean quotient
    mem_pool_create(pool, memory, chunk_size, chunk_count);
}

void *
    mem_pool_alloc(mem_pool   *pool)
{
    if (pool->free_chunk_count == 0)
        return 0;
    void *res = pool->start;
    pool->start = pool->start->next;
    pool->free_chunk_count--;
    return res;
}

void
mem_pool_dealloc(mem_pool *pool, void *ptr)
{
    pool->free_chunk_count++;
    *( (mem_pool_empy_chunk_header *)ptr ) = (mem_pool_empy_chunk_header)
    {
        .next = pool->start
    };
    pool->start = ptr;
}

void *
mem_pool_destroy(mem_pool   *pool)
{
    void *res = pool->memory;
    *pool = (mem_pool)
    {
        0
    };
    return res;
}

void
mem_pool_get_alloced_blocks(mem_pool *pool, void **block_list, u32 *count)
{
    *count = pool->chunk_count - pool->free_chunk_count;
    if (!block_list)
        return;
    if (count == 0)
        return;

    b8 *is_free = alloca(sizeof(b8) * pool->chunk_count);
    mem_memset(is_free, 0, sizeof(b8) * pool->chunk_count);

    mem_pool_empy_chunk_header *prev = pool->start;
    for (int i = 0; i < pool->free_chunk_count; i++)
    {
        u64 chunk_index = ( ( (u64)prev - (u64)pool->memory ) / (u64)pool->chunk_size );
        is_free[chunk_index] = TRUE;
        prev                 = prev->next;
    }

    u32 c = 0;
    for (int i = 0; i < pool->chunk_count; i++)
    {
        if (!is_free[i])
        {
            block_list[c] = (void *)( (u64)pool->memory + ( (u64)pool->chunk_size * i ) );
            c++; // LOL BJARNE GOES BRRRRRRR
        }
    }
}

void *
mem_pool_index(mem_pool *pool, u64 index)
{
    if(index > pool->chunk_count)
    {
        return NULL;
    }
    return (void *)( (u64)pool->memory + (pool->chunk_size * index) );
}


u64
    handle_mgr_get_size_for_count(u64 concurrent_alloc_count, u64 managed_size)
{
    return concurrent_alloc_count * ( managed_size + sizeof(handle_mgr_block_header) );
}

b8
    handle_mgr_create(handle_mgr *mgr, void *memory, u64 memory_size, u64 managed_size)
{
    mgr->managed_size = managed_size;
    mgr->memory_size  = memory_size;
    mgr->memory       = memory;

    mgr->block_size     = sizeof(handle_mgr_block_header) + managed_size;
    mgr->block_count    = memory_size / mgr->block_size; // Euclidean quotient
    mgr->alloced_blocks = 0;

    void *ptr = memory;
    for (int i = 0; i < mgr->block_count; i++)
    {
        handle_mgr_block_header hdr =
        {
            .unique_counter = 0,
            .free           = TRUE,
        };

        (*(handle_mgr_block_header *)ptr) = hdr;

        void **ll_ptr = (void **)( (u64)ptr + sizeof(handle_mgr_block_header) );
        ptr     = (void *)( (u64)ptr + mgr->block_size );
        *ll_ptr = (i == mgr->block_count - 1) ? NULL : ptr;
    }

    mgr->first_free_block = memory;

    return TRUE;
}

handle
handle_mgr_allocate(handle_mgr   *mgr)
{
    if (mgr->first_free_block)
    {
        handle_mgr_block_header *hdr = mgr->first_free_block;
        ASSERT_MSG(hdr, "No more free blocks.");
        ASSERT_MSG(hdr->free, "Tried to alloc non free block.");
        hdr->free = FALSE;
        hdr->unique_counter++;
        if (hdr->unique_counter == 0) // Overflow case
        {
            hdr->unique_counter = 1; // Never have a unique counter of 0, otherwise handles could collide with NULL_HANDLE
        }

        // Get where the free block was pointing to
        void **next = (void **)( (u64)hdr + sizeof(handle_mgr_block_header) );
        mgr->first_free_block = *next;

        // Compose
        u16 idx     = ( (u64)hdr - (u64)mgr->memory ) / mgr->block_size;
        handle hndl = idx | (hdr->unique_counter << 16);
        mgr->alloced_blocks++;
        return hndl;
    }
    return NULL_HANDLE;
}

b8
handle_mgr_free(handle_mgr *mgr, handle hndl)
{
    if (hndl)
    {
        handle_mgr_block_header *hdr = (handle_mgr_block_header *)( ( (hndl & 0x00000FFFF) * mgr->block_size ) + (u64)mgr->memory );

        if (hdr->free)
            return FALSE;

        hdr->free = TRUE;
        mgr->alloced_blocks--;

        void **next = (void **)( (u64)hdr + sizeof(handle_mgr_block_header) );
        *next                 = mgr->first_free_block;
        mgr->first_free_block = hdr;

        return TRUE;
    }
    return FALSE;
}

void *
handle_mgr_deref(handle_mgr *mgr, handle hndl)
{
    if (hndl)
    {
        handle_mgr_block_header *hdr = (handle_mgr_block_header *)( ( (hndl & 0x00000FFFF) * mgr->block_size ) + (u64)mgr->memory );

        if (hdr->free)
            return NULL;
        if ( hdr->unique_counter != ( (hndl >> 16) & 0x00000FFFF ) )
            return NULL;

        void *ret = (void *)( (u64)hdr + sizeof(handle_mgr_block_header) );
        return ret;
    }
    return NULL;
}

u64
    handle_mgr_get_count(handle_mgr   *mgr)
{
    return mgr->alloced_blocks;
}

void *
    handle_mgr_destroy(handle_mgr   *mgr)
{
    void *tmp = mgr->memory;
    mgr->memory_size = 0;
    mgr->memory      = NULL;
    return tmp;
}


void *
    fio_read_whole_file(const char *filepath, u64 *file_size)
{
    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        return NULL;
    }

    // Get file length
    fseek(f, 0, SEEK_END);
    *file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate necessary memory
    void *data = mem_allocate(*file_size, MEMORY_TAG_FIO_DATA);

    fread(data, *file_size, 1, f);

    fclose(f);

    return data;
}

// Implementation end
// (guard_end.h) please do not include this file directly
#else // BASE_IMPL_GUARD
    #warning "Base is being implemented twice."
#endif // BASE_IMPL_GUARD
#endif // BASE_IMPLEMENTATION

