#include "logging.h"

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

void    logging_start()
{
    // TODO: Output to file
}

void    logging_stop()
{
    // TODO: Output to file
}

void    logging_msg(u32 level, u32 line, char *filename, char *message, ...)
{
    // TODO: Display filename + line on option

    printf("%s%s: ", log_level_colors[level], log_level_prefixes[level]);

    va_list arg_list;
    va_start(arg_list, message);
    vprintf(message, arg_list);
    va_end(arg_list);

    printf("%s\n", ANSI_RESET);
}

