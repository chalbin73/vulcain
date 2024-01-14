#ifndef __FEMTOLOG_H__
#define __FEMTOLOG_H__

#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#define FL_MAX_CALLBACKS 32

typedef enum
{
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    FL_LOG_LEVEL_COUNT
} fl_log_level;

typedef struct
{
    struct tm      *timestamp;
    fl_log_level    level;

    const char     *subsystem_name;
    const char     *file;
    uint32_t        line_number;
    const char     *fmt;
    va_list         args;
} fl_log_event;

typedef void (*fl_log_fn_ptr)(void *user_data, fl_log_event *ev);

void    fl_set_quiet(bool    quiet);
void    fl_set_level(fl_log_level    min_level);
void    fl_add_fd(FILE *fd, fl_log_level min_level);
void    fl_add_callback(fl_log_fn_ptr fn, void *user_data, fl_log_level min_level);
void    fl_log(fl_log_level log_level, const char *subsystem_name, const char *file, uint32_t line_number, const char *fmt, ...);


#define fl_log_trace(subsys, fmt, ...) \
        fl_log(TRACE, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#define fl_log_debug(subsys, fmt, ...) \
        fl_log(DEBUG, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#define fl_log_info(subsys, fmt, ...) \
        fl_log(INFO, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#define fl_log_warn(subsys, fmt, ...) \
        fl_log(WARN, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#define fl_log_error(subsys, fmt, ...) \
        fl_log(ERROR, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#define fl_log_fatal(subsys, fmt, ...) \
        fl_log(FATAL, subsys, __FILE__, __LINE__, fmt, __VA_ARGS__);

#endif // __FEMTOLOG_H__

#ifdef FEMTOLOG_IMPLEMENTATION

static const char * const fl_log_level_names[FL_LOG_LEVEL_COUNT] =
{
    [TRACE] = "TRACE",
    [DEBUG] = "DEBUG",
    [INFO]  = "INFO",
    [WARN]  = "WARN",
    [ERROR] = "ERROR",
    [FATAL] = "FATAL",
};

#define LOG_USE_COLOR
#ifdef LOG_USE_COLOR
static const char * const fl_log_level_colors[FL_LOG_LEVEL_COUNT] =
{
    [TRACE] = "\e[94m",
    [DEBUG] = "\e[36m",
    [INFO]  = "\e[32m",
    [WARN]  = "\e[33m",
    [ERROR] = "\e[31m",
    [FATAL] = "\e[35m",
};
#endif

typedef struct
{
    fl_log_fn_ptr    clbk;
    void            *user_data;
    fl_log_level     min_level;
} fl_write_callback;

static struct
{
    fl_write_callback    callbacks[FL_MAX_CALLBACKS];
    uint32_t             callback_count;
    int                  quiet;
    fl_log_level         min_level;
}
fl_log_ctx =
{
    0
};

#ifdef LOG_USE_COLOR
// This returns a string with colored text, and colored columns, used to color the subsystem string
static void    fl_color_columns(char *out, uint64_t out_max_size, const char *in)
{
    int was_column = in[0] != ':';
    // This could be done with only a while loop, but readability is better here
    int p = 0;
    for(uint64_t i = 0; i < out_max_size - 1 && in[i] != '\0'; i++)
    {
        if(in[i] == ':')
        {
            // \e[90m Gray
            if(!was_column)
            {
                out[p++] = '\e';
                out[p++] = '[';
                out[p++] = '3';
                out[p++] = '3';
                out[p++] = 'm';
            }
            out[p++]   = ':';
            was_column = 1;
        }
        else if(was_column)
        {
            // \e[96m : Cyan
            out[p++]   = '\e';
            out[p++]   = '[';
            out[p++]   = '9';
            out[p++]   = '0';
            out[p++]   = 'm';
            out[p++]   = in[i];
            was_column = 0;
        }
        else
        {
            out[p++] = in[i];
        }
    }
    out[p++] = '\0';
}
#endif

static void    fl_stdout_clbk(void *user_data, fl_log_event *ev)
{
    char time_buf[32];
    strftime(time_buf, 32, "%H:%M:%S", ev->timestamp);

    // Print metadata
    #ifdef LOG_USE_COLOR
    printf(
        "%s %s%5s\e[0m: ",
        time_buf,
        fl_log_level_colors[ev->level],
        fl_log_level_names[ev->level]
        );
    #else
    printf(
        "%s %5s: ",
        time_buf,
        fl_log_level_names[ev->level]
        );
    #endif

    //Print the actual text
    vprintf(ev->fmt, ev->args);

    #ifdef LOG_USE_COLOR
    char colored_subsystem[2048];
    fl_color_columns(colored_subsystem, 2048, ev->subsystem_name);

    printf(
        "\e[90m -- (%s\e[90m) %s:%d\e[0m",
        colored_subsystem,
        ev->file,
        ev->line_number
        );
    #else
    printf(
        " -- (%s) %s:%d",
        ev->subsystem_name,
        ev->file,
        ev->line_number
        );
    #endif

    printf("\n");
    fflush(stdout);
}

void    fl_fd_clbk(void *user_data, fl_log_event *ev)
{
    FILE *fd = user_data;
    char time_buf[32];
    strftime(time_buf, 32, "%H:%M:%S", ev->timestamp);

    fprintf(
        fd,
        "%s %5s: ",
        time_buf,
        fl_log_level_names[ev->level]
        );

    //Print the actual text
    vfprintf(fd, ev->fmt, ev->args);

    fprintf(
        fd,
        " -- (%s) %s:%d",
        ev->subsystem_name,
        ev->file,
        ev->line_number
        );

    fprintf(fd, "\n");
    fflush(fd);
}

void    fl_add_fd(FILE *fd, fl_log_level min_level)
{
    fl_add_callback(fl_fd_clbk, fd, min_level);
}

void    fl_add_callback(fl_log_fn_ptr fn, void *user_data, fl_log_level min_level)
{
    if(fl_log_ctx.callback_count == FL_MAX_CALLBACKS)
    {
#ifdef FL_LOG_INTERNAL
        fl_log(WARN, "femtolog::callback", __FILE__, __LINE__, "Max callbacks reached. Not adding callback");
#endif
        return;
    }

    fl_log_ctx.callbacks[fl_log_ctx.callback_count++] = (fl_write_callback)
    {
        .clbk      = fn,
        .user_data = user_data,
        .min_level = min_level,
    };
}

void    fl_set_quiet(bool    quiet)
{
    fl_log_ctx.quiet = quiet;
}

void    fl_set_level(fl_log_level    min_level)
{
    fl_log_ctx.min_level = min_level;
}

void    fl_log(fl_log_level log_level, const char *subsystem_name, const char *file, uint32_t line_number, const char *fmt, ...)
{
    fl_log_event ev =
    {
        .level          = log_level,
        .file           = file,
        .line_number    = line_number,
        .fmt            = fmt,
        .subsystem_name = subsystem_name,
    };

    // Time stuff
    time_t t = time(NULL);
    ev.timestamp = localtime(&t);

    if(!fl_log_ctx.quiet && log_level >= fl_log_ctx.min_level)
    {
        va_start(ev.args, fmt);
        fl_stdout_clbk(NULL, &ev);
        va_end(ev.args);
    }

    va_start(ev.args, fmt);
    for(uint32_t i = 0; i < fl_log_ctx.callback_count; i++)
    {
        if(log_level >= fl_log_ctx.callbacks[i].min_level)
        {
            fl_log_ctx.callbacks[i].clbk(
                fl_log_ctx.callbacks[i].user_data,
                &ev
                );
        }
    }
    va_end(ev.args);
}

#endif //FEMTOLOG_IMPLEMENTATION

