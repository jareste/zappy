#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "../time_api/time_api.h"
#include "../parse_arg/config_file.h"

static FILE *m_log_fp        = NULL;
static log_level  m_log_threshold = LOG_LEVEL_INFO;
static log_config m_log_config;

int log_init()
{
    char* options;

    parse_set_log_config(&m_log_config);

    options = m_log_config.LOG_ERASE == true ? "w+" : "a";
    m_log_threshold = m_log_config.LOG_LEVEL;
    
    m_log_fp = fopen(m_log_config.LOG_FILE_PATH, options);
    if (!m_log_fp)
    {
        perror("fopen");
        return -1;
    }
    return 0;
}

void log_close(void)
{
    if (m_log_fp)
    {
        fclose(m_log_fp);
        m_log_fp = NULL;
    }
}

void log_msg(log_level level, const char *fmt, ...)
{
    va_list args, args_copy;
    static bool inside_logger = false;

    if (!m_log_fp) return;

    /* avoid circle reference with time api!! */
    if (inside_logger)
        return;
    
    inside_logger = true;

    va_start(args, fmt);
    va_copy(args_copy, args);

    fprintf(m_log_fp, "[%ld] ", time_get_current_time_units(NULL));
    vfprintf(m_log_fp, fmt, args);
    fflush(m_log_fp);

    if ((level <= m_log_threshold) || (level == LOG_LEVEL_BOOT))
    {
        switch (level)
        {
            case LOG_LEVEL_ERROR:
                fprintf(stderr, "\033[1;31m[%ld] ", time_get_current_time_units(NULL));
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            case LOG_LEVEL_WARN: /* TODO make it yellow */
                fprintf(stderr, "\033[1;31m[%ld] ", time_get_current_time_units(NULL));
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            default:
                fprintf(stdout, "[%ld] ", time_get_current_time_units(NULL));
                vfprintf(stdout, fmt, args_copy);
                break;
        }
    }

    va_end(args_copy);
    va_end(args);
    inside_logger = false;
}
