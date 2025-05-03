/***************************/
/*        INCLUDES         */
/***************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <parse_arg.h>
#include <error_codes.h>

/***************************/
/*        DEFINES          */
/***************************/

/***************************/
/*       FUNCITONS         */
/***************************/
void check_params(t_args* args)
{
    if ((args->port < 1024) && geteuid())
    {
        fprintf(stderr, "Port must be higher than 1024 or process must be launched as root.\n");
        exit(EXIT_FAILURE);
    }
    if (args->port > 65535) /* move to constants. */
    {
        fprintf(stderr, "Port must be lower than 65535\n");
        exit(EXIT_FAILURE);
    }
    if (args->width < 10 || args->height < 10)
    {
        fprintf(stderr, "Width and height must be greater than 9\n");
        exit(EXIT_FAILURE);
    }
    if (args->nb_clients < 3)
    {
        fprintf(stderr, "Number of clients must be greater than 2\n");
        exit(EXIT_FAILURE);
    }
    if (args->time_unit < 1)
    {
        fprintf(stderr, "Time unit must be greater than 0\n");
        exit(EXIT_FAILURE);
    }
    if ((args->teams == NULL) || (args->teams[0] == NULL))
    {
        fprintf(stderr, "At least one team must be specified\n");
        exit(EXIT_FAILURE);
    }
}

int parse_args(int argc, char *argv[], t_args* args)
{
    int         opt;

    return SUCCESS;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "?hp:x:y:n:c:t:f:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
            case '?':
            case 'h':
                ZAPPY_USAGE(EXIT_SUCCESS);
                exit(0);
            default:
                ZAPPY_USAGE(EXIT_SUCCESS);
        }
    }

    check_params(args);

    return SUCCESS;
}

