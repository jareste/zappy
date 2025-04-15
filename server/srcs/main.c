#include <parse_arg.h>
#include <server.h>
#include <cJSON.h>
#include <sys/signal.h>
#include "server/ssl_al.h"
#include "server/server.h"
#include "game/game.h"
#include "time_api/time_api.h"

/*debug*/
#include <time.h>
#include <stdlib.h>

#define PORT 8674

int main_loop()
{
    int sel_ret;
    int game_ret;
    int sel_timeout;
#ifdef DEBUG
    struct timeval start_time;
    struct timeval end_time;
#endif

    sel_timeout = 0;
    while (1)
    {
#ifdef DEBUG
        gettimeofday(&start_time, NULL);
#endif

        sel_ret = server_select(sel_timeout);
        if (sel_ret == ERROR)
        {
            fprintf(stderr, "Failed to select\n");
            break;
        }

        game_ret = game_play();
        if (game_ret == ERROR)
        {
            fprintf(stderr, "Failed to play\n");
            break;
        }

#ifdef DEBUG
        gettimeofday(&end_time, NULL);
        long elapsed_us = (end_time.tv_sec - start_time.tv_sec) * 1000000L + 
                      (end_time.tv_usec - start_time.tv_usec);
        printf("Loop completed in: %ld microseconds\n", elapsed_us);
#endif

        if (sel_ret == 0 && game_ret == 0)
        {
            /* Release some CPU time or computer slows down */
            sel_timeout = 10000; /* 10ms */
        }
        else
        {
            /* We are busy so keep going */
            sel_timeout = 0;
        }
    }

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
    args.width = rand() % 1000 + 4;
    args.height = rand() % 1000 + 4;
    args.nb_clients = rand() % 100 + 10;
    args.nb_teams = rand() % 14 + 1;
    args.time_unit = rand() % 1000 + 1;
    printf("Randomized values:\n\tWidth='%d'\n\tHeight='%d'\n\tNb_clients='%d'\n\tTime_unit='%lu'\n",
           args.width, args.height, args.nb_clients, args.time_unit);

    /* DEBUG_END */

    if (argc < 2)
    {
        ZAPPY_USAGE(EXIT_FAILURE);
    }
    parse_args(argc, argv, &args);

    /* On failure will simply exit soooo :)
    */
    init_server(args.port, args.cert, args.key);

    time_api_init_local(args.time_unit);

    // int game_init(int width, int height, int nb_clients, char **teams, int time_unit)

    game_init(args.width, args.height, args.teams, args.nb_clients);

    /* if server closes us something weird could happen */
    signal(SIGPIPE, SIG_IGN);

    main_loop();
    printf("Exiting...\n");

    return 0;
}