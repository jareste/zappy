#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdbool.h>

#define LOG_FILE "log.txt"

typedef enum
{
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_BOOT  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_INFO  = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5,   /* ultra-verbose: raw frame bytes, every event tick */
} log_level;

typedef struct
{
    log_level   LOG_LEVEL;
    char*       LOG_FILE_PATH;
    bool        LOG_ERASE;
} log_config;

/* ------------------------------------------------------------------ */
/*  Core API                                                           */
/* ------------------------------------------------------------------ */

int  log_init(void);
void log_close(void);
void log_msg(log_level level, const char *fmt, ...);

/* ------------------------------------------------------------------ */
/*  Domain-specific structured helpers                                 */
/*  All emit at DEBUG unless noted; filtered by the threshold.         */
/* ------------------------------------------------------------------ */

/* Network / connection lifecycle */
void log_net_connect(int fd, const char *remote_ip);
void log_net_disconnect(int fd, const char *reason);
void log_net_send(int fd, const char *json_payload);
void log_net_recv(int fd, const char *json_payload);

/* WebSocket frame logging (TRACE level) */
void log_ws_frame_in(int fd, int opcode, size_t payload_len, bool masked);
void log_ws_frame_out(int fd, int opcode, size_t payload_len);

/* SSL/TLS handshake */
void log_ssl_handshake_start(int fd);
void log_ssl_handshake_done(int fd, long elapsed_us);
void log_ssl_handshake_fail(int fd, const char *reason);

/* Auth / login */
void log_auth_attempt(int fd, const char *role, const char *team);
void log_auth_ok(int fd, const char *role, const char *team);
void log_auth_fail(int fd, const char *reason);

/* Game – player lifecycle */
void log_player_spawn(int fd, int team_id, const char *team_name, int x, int y, int dir);
void log_player_die(int fd, int team_id, const char *team_name, int x, int y, unsigned long die_time, unsigned long now);
void log_player_levelup(int fd, int old_level, int new_level);

/* Game – command scheduling & execution */
void log_cmd_received(int fd, const char *cmd, const char *arg);
void log_cmd_scheduled(int fd, const char *cmd, int delay, unsigned long exec_time);
void log_cmd_executed(int fd, const char *cmd, const char *result);
void log_cmd_rejected(int fd, const char *cmd, const char *reason);
void log_cmd_queue_full(int fd, const char *cmd);

/* Game – events */
void log_event_buffer(int fd, int count, int head, int tail, unsigned long next_exec);
void log_egg_fork(int player_fd, int egg_id, int team_id, int x, int y, unsigned long hatch_at);
void log_egg_hatch(int egg_id, int team_id, int new_player_id, int x, int y);
void log_egg_claim(int egg_player_id, int claimer_fd, const char *team_name);

/* Game – incantation */
void log_incantation_start(int initiator_fd, int level, int x, int y, int player_count);
void log_incantation_result(int initiator_fd, int level, const char *result);

/* Game – broadcast */
void log_broadcast(int emitter_fd, int x, int y, const char *text);

/* Game – resources */
void log_resource_spawn_cycle(int tiles_updated, unsigned long next_cycle);

/* Time API */
void log_time_paused(unsigned long at_units);
void log_time_resumed(unsigned long at_units, long paused_duration_ms);
void log_time_tick(unsigned long old_units, unsigned long new_units);

/* Observer */
void log_observer_connect(int fd);
void log_observer_snapshot_sent(int fd, size_t bytes);

/* ------------------------------------------------------------------ */
/*  Convenience macros                                                 */
/* ------------------------------------------------------------------ */
#define LOG_TRACE(...) log_msg(LOG_LEVEL_TRACE, __VA_ARGS__)
#define LOG_DBG(...)   log_msg(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  log_msg(LOG_LEVEL_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  log_msg(LOG_LEVEL_WARN,  __VA_ARGS__)
#define LOG_ERR(...)   log_msg(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif /* LOG_H */
