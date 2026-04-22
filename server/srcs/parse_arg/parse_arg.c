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

#define MAX_PARSED_TEAMS 16

static int m_parse_positive_int(const char *s, int *out)
{
    long val;
    char *endptr;

    if (!s || !*s)
        return ERROR;
    errno = 0;
    endptr = NULL;
    val = strtol(s, &endptr, 10);
    if (errno != 0 || endptr == s || *endptr != '\0' || val <= 0 || val > 65535)
        return ERROR;
    *out = (int)val;
    return SUCCESS;
}

static int m_parse_team_names(int argc, char *argv[], t_args *args, const char *first_team)
{
    static char *parsed_teams[MAX_PARSED_TEAMS];
    int count;

    if (!first_team || !*first_team)
        return ERROR;

    count = 0;
    parsed_teams[count++] = (char *)first_team;
    while (optind < argc && argv[optind] && argv[optind][0] != '-')
    {
        if (count >= (MAX_PARSED_TEAMS - 1))
            return ERROR;
        parsed_teams[count++] = argv[optind++];
    }
    parsed_teams[count] = NULL;
    args->teams = parsed_teams;
    args->nb_teams = (uint16_t)count;
    return SUCCESS;
}

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
    int opt;
    int parsed_port;
    
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    if (argc <= 1)
        ZAPPY_USAGE(EXIT_FAILURE);

    if (argc == 2 && m_parse_positive_int(argv[1], &parsed_port) == SUCCESS)
    {
        args->port = parsed_port;
        check_params(args);
        return SUCCESS;
    }

    while ((opt = getopt_long(argc, argv, "hp:x:y:n:t:c:f:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'p':
                args->port = atoi(optarg);
                break;
            case 'x':
                args->width = atoi(optarg);
                break;
            case 'y':
                args->height = atoi(optarg);
                break;
            case 'n':
                if (m_parse_team_names(argc, argv, args, optarg) == ERROR)
                    ZAPPY_USAGE(EXIT_FAILURE);
                break;
            case 't':
                args->time_unit = atoi(optarg);
                break;
            case 'c':
                if (m_parse_positive_int(optarg, &parsed_port) == ERROR)
                    ZAPPY_USAGE(EXIT_FAILURE);
                args->nb_clients = parsed_port;
                break;
            case 'f':
                // Config file - but you already loaded it
                break;
            case 'h':
                ZAPPY_USAGE(EXIT_SUCCESS);
                break;
            default:
                ZAPPY_USAGE(EXIT_FAILURE);
        }
    }

    check_params(args);
    return SUCCESS;
}
