#include "log.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "../time_api/time_api.h"
#include "../parse_arg/config_file.h"

static FILE      *m_log_fp        = NULL;
static log_level  m_log_threshold = LOG_LEVEL_INFO;
static log_config m_log_config;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static const char *level_tag(log_level level)
{
    switch (level)
    {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_BOOT:  return "BOOT ";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default:              return "?????";
    }
}

static const char *level_color(log_level level)
{
    switch (level)
    {
        case LOG_LEVEL_ERROR: return "\033[1;31m"; /* bold red    */
        case LOG_LEVEL_BOOT:  return "\033[1;35m"; /* bold magenta*/
        case LOG_LEVEL_WARN:  return "\033[1;33m"; /* bold yellow */
        case LOG_LEVEL_INFO:  return "\033[0;37m"; /* white       */
        case LOG_LEVEL_DEBUG: return "\033[0;36m"; /* cyan        */
        case LOG_LEVEL_TRACE: return "\033[0;90m"; /* dark grey   */
        default:              return "\033[0m";
    }
}

/* ------------------------------------------------------------------ */
/*  Core init / close / log_msg                                        */
/* ------------------------------------------------------------------ */

int log_init(void)
{
    const char *options;

    parse_set_log_config(&m_log_config);

    options = m_log_config.LOG_ERASE ? "w+" : "a";
    m_log_threshold = m_log_config.LOG_LEVEL;

    m_log_fp = fopen(m_log_config.LOG_FILE_PATH, options);
    if (!m_log_fp)
    {
        perror("log_init: fopen");
        return -1;
    }

    /* Write a session separator to the file so multiple runs are easy to tell apart */
    fprintf(m_log_fp,
        "\n================ SESSION START ================\n\n");
    fflush(m_log_fp);

    return 0;
}

void log_close(void)
{
    if (m_log_fp)
    {
        fprintf(m_log_fp,
            "\n================ SESSION END   ================\n\n");
        fflush(m_log_fp);
        fclose(m_log_fp);
        m_log_fp = NULL;
    }
}

void log_msg(log_level level, const char *fmt, ...)
{
    va_list args, args_copy;
    static bool inside_logger = false;
    unsigned long tu;

    if (!m_log_fp) return;

    /* avoid circular reference with time API */
    if (inside_logger) return;
    inside_logger = true;

    tu = time_get_current_time_units(NULL);

    va_start(args, fmt);
    va_copy(args_copy, args);

    /* Always write everything to the file with full context */
    fprintf(m_log_fp, "[%lu][%s] ", tu, level_tag(level));
    vfprintf(m_log_fp, fmt, args);
    fflush(m_log_fp);

    /* Terminal: filtered by threshold (BOOT always shown) */
    if (level <= m_log_threshold || level == LOG_LEVEL_BOOT)
    {
        FILE *out = (level <= LOG_LEVEL_WARN) ? stderr : stdout;
        fprintf(out, "%s[%lu][%s] ", level_color(level), tu, level_tag(level));
        vfprintf(out, fmt, args_copy);
        fprintf(out, "\033[0m");
        fflush(out);
    }

    va_end(args_copy);
    va_end(args);
    inside_logger = false;
}

/* ------------------------------------------------------------------ */
/*  Network / connection lifecycle                                     */
/* ------------------------------------------------------------------ */

void log_net_connect(int fd, const char *remote_ip)
{
    log_msg(LOG_LEVEL_INFO,
        "[NET][CONNECT] fd=%d remote=%s\n",
        fd, remote_ip ? remote_ip : "?");
}

void log_net_disconnect(int fd, const char *reason)
{
    log_msg(LOG_LEVEL_INFO,
        "[NET][DISCONNECT] fd=%d reason='%s'\n",
        fd, reason ? reason : "unknown");
}

void log_net_send(int fd, const char *json_payload)
{
    log_msg(LOG_LEVEL_TRACE,
        "[NET][SEND] fd=%d payload=%s\n",
        fd, json_payload ? json_payload : "<null>");
}

void log_net_recv(int fd, const char *json_payload)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[NET][RECV] fd=%d payload=%s\n",
        fd, json_payload ? json_payload : "<null>");
}

/* ------------------------------------------------------------------ */
/*  WebSocket frame logging                                            */
/* ------------------------------------------------------------------ */

void log_ws_frame_in(int fd, int opcode, size_t payload_len, bool masked)
{
    const char *opname;
    switch (opcode)
    {
        case 0x0: opname = "CONTINUATION"; break;
        case 0x1: opname = "TEXT";         break;
        case 0x2: opname = "BINARY";       break;
        case 0x8: opname = "CLOSE";        break;
        case 0x9: opname = "PING";         break;
        case 0xA: opname = "PONG";         break;
        default:  opname = "RESERVED";     break;
    }
    log_msg(LOG_LEVEL_TRACE,
        "[WS][FRAME_IN] fd=%d opcode=0x%x(%s) payload=%zu masked=%d\n",
        fd, opcode, opname, payload_len, masked);
}

void log_ws_frame_out(int fd, int opcode, size_t payload_len)
{
    log_msg(LOG_LEVEL_TRACE,
        "[WS][FRAME_OUT] fd=%d opcode=0x%x payload=%zu\n",
        fd, opcode, payload_len);
}

/* ------------------------------------------------------------------ */
/*  SSL/TLS handshake                                                  */
/* ------------------------------------------------------------------ */

void log_ssl_handshake_start(int fd)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[SSL][HANDSHAKE_START] fd=%d\n", fd);
}

void log_ssl_handshake_done(int fd, long elapsed_us)
{
    log_msg(LOG_LEVEL_INFO,
        "[SSL][HANDSHAKE_DONE] fd=%d elapsed=%ldus\n", fd, elapsed_us);
}

void log_ssl_handshake_fail(int fd, const char *reason)
{
    log_msg(LOG_LEVEL_ERROR,
        "[SSL][HANDSHAKE_FAIL] fd=%d reason='%s'\n",
        fd, reason ? reason : "unknown");
}

/* ------------------------------------------------------------------ */
/*  Auth / login                                                       */
/* ------------------------------------------------------------------ */

void log_auth_attempt(int fd, const char *role, const char *team)
{
    log_msg(LOG_LEVEL_INFO,
        "[AUTH][ATTEMPT] fd=%d role='%s' team='%s'\n",
        fd,
        role ? role : "?",
        team ? team : "N/A");
}

void log_auth_ok(int fd, const char *role, const char *team)
{
    log_msg(LOG_LEVEL_INFO,
        "[AUTH][OK] fd=%d role='%s' team='%s'\n",
        fd,
        role ? role : "?",
        team ? team : "N/A");
}

void log_auth_fail(int fd, const char *reason)
{
    log_msg(LOG_LEVEL_WARN,
        "[AUTH][FAIL] fd=%d reason='%s'\n",
        fd, reason ? reason : "unknown");
}

/* ------------------------------------------------------------------ */
/*  Game – player lifecycle                                            */
/* ------------------------------------------------------------------ */

static const char *dir_name(int dir)
{
    switch (dir)
    {
        case 0: return "N";
        case 1: return "E";
        case 2: return "S";
        case 3: return "W";
        default: return "?";
    }
}

void log_player_spawn(int fd, int team_id, const char *team_name, int x, int y, int dir)
{
    log_msg(LOG_LEVEL_INFO,
        "[PLAYER][SPAWN] fd=%d team=%d('%s') pos=(%d,%d) dir=%s\n",
        fd, team_id, team_name ? team_name : "?", x, y, dir_name(dir));
}

void log_player_die(int fd, int team_id, const char *team_name,
                    int x, int y, unsigned long die_time, unsigned long now)
{
    log_msg(LOG_LEVEL_INFO,
        "[PLAYER][DIE] fd=%d team=%d('%s') pos=(%d,%d) die_time=%lu now=%lu\n",
        fd, team_id, team_name ? team_name : "?", x, y, die_time, now);
}

void log_player_levelup(int fd, int old_level, int new_level)
{
    log_msg(LOG_LEVEL_INFO,
        "[PLAYER][LEVELUP] fd=%d level=%d->%d\n",
        fd, old_level, new_level);
}

/* ------------------------------------------------------------------ */
/*  Game – command scheduling & execution                              */
/* ------------------------------------------------------------------ */

void log_cmd_received(int fd, const char *cmd, const char *arg)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[CMD][RECV] fd=%d cmd='%s' arg='%s'\n",
        fd, cmd ? cmd : "?", arg ? arg : "");
}

void log_cmd_scheduled(int fd, const char *cmd, int delay, unsigned long exec_time)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[CMD][SCHED] fd=%d cmd='%s' delay=%d exec_at=%lu\n",
        fd, cmd ? cmd : "?", delay, exec_time);
}

void log_cmd_executed(int fd, const char *cmd, const char *result)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[CMD][EXEC] fd=%d cmd='%s' result='%s'\n",
        fd, cmd ? cmd : "?", result ? result : "ok");
}

void log_cmd_rejected(int fd, const char *cmd, const char *reason)
{
    log_msg(LOG_LEVEL_WARN,
        "[CMD][REJECT] fd=%d cmd='%s' reason='%s'\n",
        fd, cmd ? cmd : "?", reason ? reason : "unknown");
}

void log_cmd_queue_full(int fd, const char *cmd)
{
    log_msg(LOG_LEVEL_WARN,
        "[CMD][QUEUE_FULL] fd=%d cmd='%s' — event buffer full, command dropped\n",
        fd, cmd ? cmd : "?");
}

/* ------------------------------------------------------------------ */
/*  Game – event buffer diagnostics                                    */
/* ------------------------------------------------------------------ */

void log_event_buffer(int fd, int count, int head, int tail, unsigned long next_exec)
{
    log_msg(LOG_LEVEL_TRACE,
        "[EVT][BUFFER] fd=%d count=%d head=%d tail=%d next_exec=%lu\n",
        fd, count, head, tail, next_exec);
}

/* ------------------------------------------------------------------ */
/*  Game – egg / fork                                                  */
/* ------------------------------------------------------------------ */

void log_egg_fork(int player_fd, int egg_id, int team_id, int x, int y, unsigned long hatch_at)
{
    log_msg(LOG_LEVEL_INFO,
        "[EGG][FORK] player_fd=%d egg_id=%d team=%d pos=(%d,%d) hatch_at=%lu\n",
        player_fd, egg_id, team_id, x, y, hatch_at);
}

void log_egg_hatch(int egg_id, int team_id, int new_player_id, int x, int y)
{
    log_msg(LOG_LEVEL_INFO,
        "[EGG][HATCH] egg_id=%d team=%d new_player_id=%d pos=(%d,%d)\n",
        egg_id, team_id, new_player_id, x, y);
}

void log_egg_claim(int egg_player_id, int claimer_fd, const char *team_name)
{
    log_msg(LOG_LEVEL_INFO,
        "[EGG][CLAIM] pre_assigned_id=%d claimer_fd=%d team='%s'\n",
        egg_player_id, claimer_fd, team_name ? team_name : "?");
}

/* ------------------------------------------------------------------ */
/*  Game – incantation                                                 */
/* ------------------------------------------------------------------ */

void log_incantation_start(int initiator_fd, int level, int x, int y, int player_count)
{
    log_msg(LOG_LEVEL_INFO,
        "[INCANT][START] initiator_fd=%d level=%d pos=(%d,%d) players_on_tile=%d\n",
        initiator_fd, level, x, y, player_count);
}

void log_incantation_result(int initiator_fd, int level, const char *result)
{
    log_msg(LOG_LEVEL_INFO,
        "[INCANT][RESULT] initiator_fd=%d level=%d result='%s'\n",
        initiator_fd, level, result ? result : "?");
}

/* ------------------------------------------------------------------ */
/*  Game – broadcast                                                   */
/* ------------------------------------------------------------------ */

void log_broadcast(int emitter_fd, int x, int y, const char *text)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[BROADCAST] emitter_fd=%d pos=(%d,%d) text='%.80s%s'\n",
        emitter_fd, x, y,
        text ? text : "",
        (text && strlen(text) > 80) ? "..." : "");
}

/* ------------------------------------------------------------------ */
/*  Game – resource spawning                                           */
/* ------------------------------------------------------------------ */

void log_resource_spawn_cycle(int tiles_updated, unsigned long next_cycle)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[RESOURCES][SPAWN] tiles_updated=%d next_cycle_at=%lu\n",
        tiles_updated, next_cycle);
}

/* ------------------------------------------------------------------ */
/*  Time API                                                           */
/* ------------------------------------------------------------------ */

void log_time_paused(unsigned long at_units)
{
    log_msg(LOG_LEVEL_INFO,
        "[TIME][PAUSED] at_units=%lu\n", at_units);
}

void log_time_resumed(unsigned long at_units, long paused_duration_ms)
{
    log_msg(LOG_LEVEL_INFO,
        "[TIME][RESUMED] at_units=%lu paused_for=%ldms\n",
        at_units, paused_duration_ms);
}

void log_time_tick(unsigned long old_units, unsigned long new_units)
{
    log_msg(LOG_LEVEL_TRACE,
        "[TIME][TICK] %lu -> %lu (diff=%lu)\n",
        old_units, new_units, new_units - old_units);
}

/* ------------------------------------------------------------------ */
/*  Observer                                                           */
/* ------------------------------------------------------------------ */

void log_observer_connect(int fd)
{
    log_msg(LOG_LEVEL_INFO,
        "[OBS][CONNECT] fd=%d\n", fd);
}

void log_observer_snapshot_sent(int fd, size_t bytes)
{
    log_msg(LOG_LEVEL_DEBUG,
        "[OBS][SNAPSHOT] fd=%d bytes=%zu\n", fd, bytes);
}
