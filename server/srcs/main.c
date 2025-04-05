#include <parse_arg.h>
#include <server.h>

int main(int argc, char **argv)
{
    // t_args args;
    (void)argv;

    if (argc < 2)
    {
        ZAPPY_USAGE(EXIT_FAILURE);
    }

    // parse_args(argc, argv, &args);
    socket_main();

    return 0;
}