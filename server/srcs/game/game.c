#include <ft_malloc.h>
#include <error_codes.h>
#include <ft_list.h>

#define PLAYER_CAN_PLAY(x) (x->player.life_units > 0 && x->player.level > 0)
#define CLIENT_HAS_ACTIONS(x, current_time) (x->event_buffer.count > 0 &&\
    x->event_buffer[x->event_buffer.head].exec_time <= current_time)

int init_game(int width, int height, int nb_clients, char **teams, int time_unit)
{
    /* initialize game structures and players */
    return SUCCESS;
}

int play()
{
    /* check if players can play and then make them play */
    return SUCCESS;
}
