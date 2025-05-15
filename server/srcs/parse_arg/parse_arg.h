#ifndef PARSE_ARG_H
#define PARSE_ARG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>

#define ZAPPY_USAGE(x) do{ \
    printf("Usage: zappy [--help] -p <port> -x <world width>"\
        "-y <world height> -n <team> [<team>] [<team>] ... -c <nb> "\
        "-t <t> -f <file>\n");\
    printf("Options:\n");\
    printf("  -h, --help\t\t\t\tDisplay this help message\n");\
    printf("  -p <port>\t\t\t\tPort number\n");\
    printf("  -x <world width>\t\t\tWorld width\n");\
    printf("  -y <world height>\t\t\tWorld height\n");\
    printf("  -n <team> [<team>] [<team>] ...\tTeams that will exist\n");\
    printf("  -c <nb>\t\t\t\tNumber of clients authorized at the beginning of the game\n");\
    printf("  -t <t>\t\t\t\tTime unit divider (the greater t is, the faster the game will go)\n");\
    printf("  -f <file>\t\t\t\tFile name of a json with the server configuration. Will override any other flag\n");\
    exit(x);\
} while(0)

typedef struct s_args
{
    int         port;
    uint32_t    width;
    uint32_t    height;
    char**      teams; /* this will be a list while parsing, no sense to be reallocating or whatever. */
    uint16_t    nb_teams; /* no sense to have more than 65535 teams being honest, even 65535 are way too many. */
    uint16_t    nb_clients; /* no sense to have more than 65535 clients being honest, even 65535 are way too many. */
    time_t      time_unit;
    char*       cert; /* cert for SSL */
    char*       key; /* key for SSL */
} t_args;

int parse_args(int argc, char *argv[], t_args* args);

#endif /* PARSE_ARG_H */
