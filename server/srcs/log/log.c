#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "../time_api/time_api.h"

static FILE *m_log_fp        = NULL;
static log_level_t  m_log_threshold = LOG_LEVEL_INFO;

int log_init(log_level_t threshold)
{
    m_log_threshold = threshold;
    
    m_log_fp = fopen(LOG_FILE, "w+");
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

void log_msg(log_level_t level, const char *fmt, ...)
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

    fprintf(m_log_fp, "[%d] ", time_get_current_time_units(NULL));
    vfprintf(m_log_fp, fmt, args);
    fflush(m_log_fp);

    if ((level <= m_log_threshold) || (level == LOG_LEVEL_BOOT))
    {
        switch (level)
        {
            case LOG_LEVEL_ERROR:
                fprintf(stderr, "\033[1;31m[%d] ", time_get_current_time_units(NULL));
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            case LOG_LEVEL_WARN: /* TODO make it yellow */
                fprintf(stderr, "\033[1;31m[%d] ", time_get_current_time_units(NULL));
                vfprintf(stderr, fmt, args_copy);
                fprintf(stderr, "\033[0m");
                break;
            default:
                fprintf(stdout, "[%d] ", time_get_current_time_units(NULL));
                vfprintf(stdout, fmt, args_copy);
                break;
        }
    }

    va_end(args_copy);
    va_end(args);
    inside_logger = false;
}
