#include <parse_arg.h>
#include <server.h>
#include <cJSON.h>
#include "server/ssl_al.h"
#include "server/server.h"
#include "game/game.h"

#define PORT 8674

int main_loop()
{
    int ret;

    /* On failure will simply exit soooo :)
    */
    init_server(PORT);

    while (1)
    {

        ret = server_select();
        if (ret == ERROR)
        {
            fprintf(stderr, "Failed to select\n");
            break;
        }

        ret = play();
        if (ret == ERROR)
        {
            fprintf(stderr, "Failed to play\n");
            break;
        }
    }

    cleanup_server();
    cleanup_ssl_al();
    return 0;
}

int main(int argc, char **argv)
{
    // t_args args;
    (void)argv;

    if (argc < 2)
    {
        ZAPPY_USAGE(EXIT_FAILURE);
    }

    // parse_args(argc, argv, &args);

    main_loop();

    return 0;
}