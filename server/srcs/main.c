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

/*debug*/
#include <time.h>
#include <stdlib.h>

#define PORT 8674

static bool m_die = false;

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        m_die = true;
    }
}

int main_loop()
{
    int sel_ret;
    int game_ret;
    int initial_time_units;
    int _initial_time_units;
    int final_time_units;
    int _final_time_units;
    int time_to_select;
    int time_to_play;

    while (!m_die)
    {
        time_api_update(NULL);
        {
            initial_time_units = time_api_get_local()->current_time_units;
            _initial_time_units = initial_time_units;
        }

        sel_ret = server_select();
        if (sel_ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to select\n");
            break;
        }
        time_api_update(NULL);

        {
            final_time_units = time_api_get_local()->current_time_units;

            time_to_select = final_time_units - initial_time_units;

            initial_time_units = time_api_get_local()->current_time_units;
        }

        game_ret = game_play();
        if (game_ret == ERROR)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to play\n");
            break;
        }

        {
            time_api_update(NULL);
            final_time_units = time_get_current_time_units(NULL);

            time_to_play = final_time_units - initial_time_units;

            if ((time_to_select + time_to_play) > 2)
            {
                log_msg(LOG_LEVEL_WARN, "Server exhausted, might lose some events. Selected(%d) in '%d'. Played in '%d' (%d)\n", 
                    sel_ret, time_to_select, time_to_play, time_get_current_time_units(NULL));
            }

            _final_time_units = time_get_current_time_units(NULL);
            if (_final_time_units - _initial_time_units > 5)
            {
                log_msg(LOG_LEVEL_DEBUG, "Loop time deviation (%d) (%d)\n", 
                    _final_time_units - _initial_time_units, time_get_current_time_units(NULL));
            }
        }
    }

    game_clean();
    time_api_free(NULL);
    cleanup_server();
    cleanup_ssl_al();
    return 0;
}

char* teams[] = {"team1", "team2", "team3", "team4", "team5",\
                  "team6", "team7", "team8", "team9", "team10",
                  "team11", "team12", "team13", "team14", "team15",
                  NULL};

int main(int argc, char **argv)
{
    /* DEBUG */
    t_args args = {
        .port = PORT,
        .width = 10,
        .height = 10,
        .teams = teams,
        .nb_teams = 2,
        .nb_clients = 3,
        .time_unit = 1,
        .cert = "certs/cert.pem",
        .key = "certs/key.pem",
    };

    srand(time(NULL));
    // args.width = rand() % 1000 + 4;
    // args.width = 10000;
    args.width = 10;
    // args.height = rand() % 1000 + 4;
    // args.height = 10000;
    args.height = 10;
    args.nb_clients = 1000;
    // args.nb_teams = rand() % 14 + 1;
    args.nb_teams = 2;
    // args.time_unit = rand() % 1000 + 1;
    args.time_unit = 800;
    
    if (parse_config("config") == ERROR)
            goto error;

    log_init(LOG_LEVEL_WARN);

    log_msg(LOG_LEVEL_BOOT, "Randomized values:\n\tWidth='%d'\n\tHeight='%d'\n\tNb_clients='%d'\n\tTime_unit='%lu'\n",
           args.width, args.height, args.nb_clients, args.time_unit);

    int port = atoi(argc>1?argv[1]:"2");
    if (port != 2)
    {
        args.port = port;
    }
    else
    {
        args.port = PORT;
    }
    /* DEBUG_END */

    if (args.nb_teams > args.nb_clients)
    {
        log_msg(LOG_LEVEL_ERROR, "ERROR: there could not be more teams than clients.\n");
        return ERROR;
    }

    if (argc < 2)
    {
        ZAPPY_USAGE(EXIT_FAILURE);
    }
    if (parse_args(argc, argv, &args) == ERROR)
        goto error;

    /* On failure will simply exit soooo :)
    */
    if (init_server(args.port, args.cert, args.key) == ERROR)
        goto error;

    if (time_api_init_local(args.time_unit) == ERROR)
        goto error;

    if (game_init(args.width, args.height, args.teams,\
     args.nb_clients, args.nb_teams) == ERROR)
        goto error;

    parse_free_config();

    /* if server closes us something weird could happen */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    main_loop();
    log_msg(LOG_LEVEL_INFO, "Exiting...\n");
    log_close();

    return 0;

error:
    parse_free_config();
    game_clean();
    time_api_free(NULL);
    cleanup_server();
    cleanup_ssl_al();
    return EXIT_FAILURE;
}