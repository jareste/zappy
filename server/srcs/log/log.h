#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdbool.h>

#define LOG_FILE "log.txt"

typedef enum
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_BOOT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level;

typedef struct
{
    log_level LOG_LEVEL;
    char* LOG_FILE_PATH;
    bool LOG_ERASE;
} log_config;

int log_init();

void log_close(void);

void log_msg(log_level level, const char *fmt, ...);

#endif
