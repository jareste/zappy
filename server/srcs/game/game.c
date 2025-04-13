#include <ft_malloc.h>
#include <error_codes.h>
#include <ft_list.h>
#include <string.h>
#include "game_structs.h"
#include "../time_api/time_api.h"

#define PLAYER_IS_ALIVE(x, current_time) ((x->player->die_time > current_time))
#define CLIENT_HAS_ACTIONS(x, current_time) \
    ((x->event_buffer.count > 0) && (x->event_buffer.events[x->event_buffer.head].exec_time <= current_time))

server m_server = {0};

int m_game_init_team(team *team, char *name, int max_players)
{
    team->name = strdup(name);
    team->max_players = max_players;
    team->current_players = 0;
    team->players = malloc(sizeof(player*) * max_players);
    memset(team->players, 0, sizeof(player*) * max_players);
    return SUCCESS;
}

int game_init(int width, int height, char **teams, int nb_clients)
{
    int team_number;
    int i;
    int ret;

    i = 0;
    while (teams[i])
        i++;

    m_server.map_x = width;
    m_server.map_y = height;
    m_server.map = malloc(sizeof(tile*) * width);
    memset(m_server.map, 0, sizeof(tile*) * width);
    m_server.teams = malloc(sizeof(team) * nb_clients);
    memset(m_server.teams, 0, sizeof(team) * nb_clients);
    m_server.clients = malloc(sizeof(client*) * nb_clients);
    memset(m_server.clients, 0, sizeof(client*) * nb_clients);
    m_server.client_count = nb_clients;
    m_server.team_count = i;
    
    i = 0;
    team_number = 0;
    while (teams[i])
    {
        ret = m_game_init_team(&m_server.teams[team_number], teams[i], m_server.team_count / nb_clients);
        team_number++;
        i++;
    }

    return SUCCESS;
}

int game_register_player(int fd, char *team_name)
{
    /* register player in the game */
    (void)fd;
    (void)team_name;
    return SUCCESS;
}

int game_execute_command(int fd, char *cmd, char *arg)
{
    /* this must queue the command to be executed later on */
    (void)fd;
    (void)cmd;
    (void)arg;
    return SUCCESS;
}

void game_player_die(client *c)
{
    /* this must remove the player from the game */
    /* and free the memory */
    /* and send a message to the client */
    /* and remove the player from the team */
    /* and remove the player from the map */
    /* and remove the player from the server */
    /* and remove the player from the event buffer */
    (void)c;
}
#include <stdio.h>
int game_play()
{
    int i;
    time_api* t_api;
    client* c;

    time_api_update(NULL);
    t_api = time_api_get_local();
    printf("Current time units: %d\n", t_api->current_time_units);
    i = 0;
    while (i < m_server.client_count)
    {
        c = m_server.clients[i];
        i++;

        /**/
        if (c == NULL)
            continue; /* No client */

        if (!PLAYER_IS_ALIVE(c, t_api->current_time_units))
            game_player_die(c); /* Player not ready */

        if (CLIENT_HAS_ACTIONS(c, t_api->current_time_units))
            time_api_process_client_events(NULL, &c->event_buffer);
    }


    /* check if players can play and then make them play */
    return SUCCESS;
}
