#pragma once

#include "ansi.h"
#include "types.h"
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

