#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_FILE "log.txt"

typedef enum
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_BOOT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level_t;

int log_init(log_level_t threshold);

void log_close(void);

void log_msg(log_level_t level, const char *fmt, ...);

#endif
