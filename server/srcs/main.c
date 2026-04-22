#include <parse_arg.h>
#include <server.h>
#include <cJSON.h>
#include <sys/signal.h>
#include <stdbool.h>
#include "server/ssl_al.h"
#include "server/server.h"
#include "game/game.h"
#include "log/log.h"
#include "time_api/time_api.h"
#include "parse_arg/config_file.h"

#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define PORT 8674

static bool m_die  = false;
static bool m_play = false;

void signal_handler(int signum)
{
    log_msg(LOG_LEVEL_WARN,
        "[SIGNAL] Received signal %d (SIGINT=%d SIGTERM=%d SIGUSR1=%d)\n",
        signum, SIGINT, SIGTERM, SIGUSR1);

    if (signum == SIGINT || signum == SIGTERM)
    {
        m_die = true;
    }
    if (signum == SIGUSR1)
    {
        m_play = !m_play;
        if (m_play)
        {
            time_api_run(NULL);
            log_time_resumed(time_api_get_local()->current_time_units, 0);
            log_msg(LOG_LEVEL_INFO, "[SIGNAL] Game STARTED via SIGUSR1 — time API resumed\n");
        }
        else
        {
            time_api_pause(NULL);
            log_time_paused(time_api_get_local()->current_time_units);
            log_msg(LOG_LEVEL_INFO, "[SIGNAL] Game PAUSED via SIGUSR1 — time API paused\n");
        }
    }
}

int main_loop(void)
{
    int      sel_ret;
    int      game_ret;
    int      initial_time_units;
    int      _initial_time_units;
    int      final_time_units;
    int      time_to_select;
    int      time_to_play;

    log_msg(LOG_LEVEL_BOOT,
        "[MAIN_LOOP] Entering main loop — time API is PAUSED, waiting for SIGUSR1 to start\n");

    while (!m_die)
    {
        static int64_t last_log   = 0;
        int64_t        now_ms     = get_current_time_ms();

        if (now_ms - last_log > 5000)
        {
            log_msg(LOG_LEVEL_INFO,
                "[MAIN_LOOP] Heartbeat — time_units=%lu paused=%s\n",
                time_api_get_local()->current_time_units,
                (time_api_get_local()->paused_ms > 0) ? "yes" : "no");
            last_log = now_ms;
        }

        time_api_update(NULL);

        {
            initial_time_units  = time_api_get_local()->current_time_units;
            _initial_time_units = initial_time_units;
        }

        sel_ret = server_select();
        if (sel_ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "[MAIN_LOOP] server_select() returned ERROR — exiting loop\n");
            break;
        }
        time_api_update(NULL);

        {
            final_time_units = time_api_get_local()->current_time_units;
            time_to_select   = final_time_units - initial_time_units;
            initial_time_units = time_api_get_local()->current_time_units;
        }

        game_ret = game_play();
        if (game_ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "[MAIN_LOOP] game_play() returned ERROR — exiting loop\n");
            break;
        }

        {
            time_api_update(NULL);
            final_time_units = time_get_current_time_units(NULL);
            time_to_play     = final_time_units - initial_time_units;

            if ((time_to_select + time_to_play) > 2)
            {
                log_msg(LOG_LEVEL_WARN,
                    "[MAIN_LOOP] Server exhausted — may lose events. "
                    "select=%d time_units, play=%d time_units, total=%d, now=%d\n",
                    time_to_select, time_to_play,
                    time_to_select + time_to_play,
                    time_get_current_time_units(NULL));
            }

            final_time_units = time_get_current_time_units(NULL);
            if (final_time_units - _initial_time_units > 5)
            {
                log_msg(LOG_LEVEL_DEBUG,
                    "[MAIN_LOOP] Loop time deviation: %d time_units (now=%d)\n",
                    final_time_units - _initial_time_units,
                    time_get_current_time_units(NULL));
            }
        }
    }

    log_msg(LOG_LEVEL_INFO, "[MAIN_LOOP] Exiting — cleaning up\n");
    game_clean();
    time_api_free(NULL);
    cleanup_server();
    cleanup_ssl_al();
    return 0;
}

char* teams[] = {
    "team1", "team2", "team3", "team4", "team5",
    "team6", "team7", "team8", "team9", "team10",
    "team11", "team12", "team13", "team14", "team15",
    NULL
};

int main(int argc, char **argv)
{
    char  *time_unit_env;
    char  *endptr;
    long   parsed_time_unit;
    int    nb_clients;

    /* Defaults */
    t_args args = {
        .port     = PORT,
        .width    = 10,
        .height   = 10,
        .nb_clients = 20,
        .teams    = teams,
        .nb_teams = 2,
        .time_unit = 100,
        .cert     = "certs/cert.pem",
        .key      = "certs/key.pem",
    };

    /* 1. CLI args (lowest priority) */
    if (parse_args(argc, argv, &args) == ERROR)
        goto error;

    /* 2. Config file overrides CLI */
    if (parse_config("config") != ERROR)
        args.time_unit = parse_get_time_unit();

    /* 3. Environment variable (highest priority) */
    time_unit_env = getenv("ZAPPY_TIME_UNIT");
    if (time_unit_env && *time_unit_env)
    {
        errno  = 0;
        endptr = NULL;
        parsed_time_unit = strtol(time_unit_env, &endptr, 10);
        if (errno == 0 && endptr != time_unit_env && *endptr == '\0' && parsed_time_unit > 0)
            args.time_unit = (time_t)parsed_time_unit;
    }

    if (args.nb_clients > 0)
        parse_override_nb_clients(args.nb_clients);

    /* Init logging first so everything that follows is captured */
    log_init();

    nb_clients = 0;
    parse_set_nb_clients(&nb_clients);

    log_msg(LOG_LEVEL_BOOT,
        "[INIT] Server configuration:\n"
        "       port=%d  width=%d  height=%d\n"
        "       nb_clients=%d  time_unit=%lu\n"
        "       cert='%s'  key='%s'\n"
        "       ZAPPY_EASY_ASCENSION=%s\n",
        args.port, args.width, args.height,
        nb_clients, (unsigned long)args.time_unit,
        args.cert, args.key,
        getenv("ZAPPY_EASY_ASCENSION") ? getenv("ZAPPY_EASY_ASCENSION") : "unset");

    if (args.nb_teams > 15)
    {
        log_msg(LOG_LEVEL_ERROR, "[INIT] nb_teams=%d exceeds maximum of 15\n", args.nb_teams);
        return ERROR;
    }

    parse_get_certificates(&args.cert, &args.key);

    if (init_server(args.port, args.cert, args.key) == ERROR)
        goto error;

    if (time_api_init_local(args.time_unit) == ERROR)
        goto error;

    /*
     * The time API starts PAUSED intentionally.
     * Start order: server boots → clients log in → GUI connects as observer
     * and sends SIGUSR1 to resume the clock and start the game.
     */
    time_api_pause(NULL);
    log_msg(LOG_LEVEL_BOOT,
        "[INIT] Time API paused — send SIGUSR1 (pid=%d) to start the game\n",
        (int)getpid());

    if (game_init(args.width, args.height, args.teams, args.nb_teams) == ERROR)
        goto error;

    parse_free_config();

    log_msg(LOG_LEVEL_BOOT,
        "[INIT] Server ready on port %d — waiting for connections\n", args.port);
    printf("Server started on port %d (pid=%d)\n", args.port, (int)getpid());

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);

    main_loop();

    log_msg(LOG_LEVEL_INFO, "[MAIN] Clean exit\n");
    log_close();
    return 0;

error:
    parse_free_config();
    game_clean();
    time_api_free(NULL);
    cleanup_server();
    cleanup_ssl_al();
    log_close();
    return EXIT_FAILURE;
}
