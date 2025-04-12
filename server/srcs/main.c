#include <parse_arg.h>
#include <server.h>
#include <cJSON.h>
#include "server/ssl_al.h"
#include "server/server.h"
#include "game/game.h"
#include "time_api/time_api.h"

/*debug*/
#include <time.h>

#define PORT 8674

int main_loop()
{
    int ret;
#ifdef DEBUG
    struct timeval start_time;
    struct timeval end_time;
#endif

    while (1)
    {
#ifdef DEBUG
        gettimeofday(&start_time, NULL);
#endif

        ret = server_select();
        if (ret == ERROR)
        {
            fprintf(stderr, "Failed to select\n");
            break;
        }

        ret = game_play();
        if (ret == ERROR)
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
    }

    cleanup_server();
    cleanup_ssl_al();
    return 0;
}

char* teams[] = {"team1", "team2", NULL};

int main(int argc, char **argv)
{
    /* DEBUG */
    t_args args = {
        .port = PORT,
        .width = 10,
        .height = 10,
        .teams = teams,
        .nb_clients = 3,
        .time_unit = 1,
        .cert = "certs/cert.pem",
        .key = "certs/key.pem",
    };
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

    main_loop();

    return 0;
}