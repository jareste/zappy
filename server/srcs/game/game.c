#include <ft_malloc.h>
#include <error_codes.h>
#include <ft_list.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "game_structs.h"
#include "../time_api/time_api.h"
#include "../server/server.h"/* not liking it*/

#define PLAYER_IS_ALIVE(x, current_time) ((x->player->die_time > current_time))
#define CLIENT_HAS_ACTIONS(x, current_time) \
    ((x->event_buffer.count > 0) && (x->event_buffer.events[x->event_buffer.head].exec_time <= current_time))

#define TEAM_IS_FULL(id) (m_server.teams[id].current_players >= m_server.teams[id].max_players)

#define TIME_TO_DIE 1260 /* Stated in the subject */

typedef enum
{
    AVANCE = 0,
    DROITE,
    GAUCHE,
    VOIR,
    INVENTAIRE,
    PREND,
    POSE,
    EXPULSE,
    BROADCAST,
    INCARNATION,
    FORK,
    CONNECT_NBR,
    MAX_COMMANDS
} command_type;

typedef struct 
{
    command_type type;
    char *name;
} command_message;

typedef int (*command_prototype)(void* p, void* arg);

typedef struct 
{
    command_prototype prototype;
    int delay;
}command;


int m_command_avance(void* p, void* arg);

server m_server = {0};

command_message command_messages[MAX_COMMANDS] =
{
    {AVANCE, "avance"},
    {DROITE, "droite"},
    {GAUCHE, "gauche"},
    {VOIR, "voir"},
    {INVENTAIRE, "inventaire"},
    {PREND, "prend"},
    {POSE, "pose"},
    {EXPULSE, "expulse"},
    {BROADCAST, "broadcast"},
    {INCARNATION, "incarnation"},
    {FORK, "fork"},
    {CONNECT_NBR, "connect_nbr"}
};

command command_prototypes[MAX_COMMANDS] =
{
    {m_command_avance, 7},
    {NULL, 7},
    {NULL, 7},
    {NULL, 7},
    {NULL, 1},
    {NULL, 7},
    {NULL, 7},
    {NULL, 7},
    {NULL, 7},
    {NULL, 6},
    {NULL, 42},
    {NULL, 42},
};


int m_game_init_team(team *team, char *name, int max_players)
{
    team->name = strdup(name);
    team->max_players = max_players;
    team->current_players = 0;
    team->players = malloc(sizeof(player*) * max_players);
    memset(team->players, 0, sizeof(player*) * max_players);
    return SUCCESS;
}

int m_game_get_team_id(char *name)
{
    int i;

    for (i = 0; i < m_server.team_count; i++)
    {
        if (strcmp(m_server.teams[i].name, name) == 0)
            return i;
    }
    return ERROR;
}

int m_game_get_start_pos(int *x, int *y, direction* dir)
{
    *x = rand() % m_server.map_x;
    *y = rand() % m_server.map_y;
    *dir = rand() % 4; /* 0 = north, 1 = east, 2 = south, 3 = west */
    return SUCCESS;
}

int m_game_get_client_from_fd(int fd, client **c)
{
    int i;

    for (i = 0; i < m_server.client_count; i++)
    {
        if (m_server.clients[i] && m_server.clients[i]->socket_fd == fd)
        {
            *c = m_server.clients[i];
            return SUCCESS;
        }
    }
    return ERROR;
}

int m_team_add_player_to_team(player *p)
{
    int team_id;
    team *t;

    team_id = p->team_id;
    t = &m_server.teams[team_id];

    if (t->current_players >= t->max_players)
        return ERROR;

    t->players[t->current_players] = p->id;
    t->current_players++;
    return SUCCESS;
}

int m_team_remove_player_from_team(player *p)
{
    int team_id;
    team *t;

    team_id = p->team_id;
    t = &m_server.teams[team_id];

    if (t->current_players <= 0)
        return ERROR;

    t->players[t->current_players] = 0;
    t->current_players--;
    return SUCCESS;
}

int m_add_client_to_server(client *c)
{
    int i;

    for (i = 0; i < m_server.client_count; i++)
    {
        if (m_server.clients[i] == NULL)
        {
            m_server.clients[i] = c;
            return SUCCESS;
        }
    }
    return ERROR;
}

int m_remove_client_from_server(client *c)
{
    int i;

    for (i = 0; i < m_server.client_count; i++)
    {
        if (m_server.clients[i] == c)
        {
            m_server.clients[i] = NULL;
            return SUCCESS;
        }
    }
    return ERROR;
}

int m_command_avance(void* _p, void* _arg)
{
    player *p;
    char *arg;

    p = (player*)_p;
    arg = (char*)_arg;
    /* move the player forward */
    /* check if the player can move */
    /* check if the player is alive */
    /* check if the player is not dead */
    (void)p;
    (void)arg;
    printf("---------------------------------------------------------------------\n");
    printf("Player %d is moving forward\n", p->id);

    return server_create_response_to_command(p->id, "avance", arg, "ok");
}

int game_get_client_count()
{
    return m_server.client_count;
}

int game_get_team_count()
{
    return m_server.team_count;
}

void game_get_map_size(int *width, int *height)
{
    *width = m_server.map_x;
    *height = m_server.map_y;
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
        ret = m_game_init_team(&m_server.teams[team_number], teams[i], m_server.client_count / m_server.team_count);
        if (ret == ERROR)
        {
            fprintf(stderr, "Failed to initialize team %s\n", teams[i]);
            return ERROR;
        }

        team_number++;
        i++;
    }

    return SUCCESS;
}

int game_register_player(int fd, char *team_name)
{
    client *c;
    player *p;
    int team_id;
    time_api *t_api;
    
    team_id = m_game_get_team_id(team_name);
    if (team_id < 0)
    {
        fprintf(stderr, "Failed to get team id for team %s\n", team_name);
        return team_id;
    }

    if (TEAM_IS_FULL(team_id))
    {
        fprintf(stderr, "Team %s is full\n", team_name);
        return ERROR;
    }

    c = malloc(sizeof(client));
    p = malloc(sizeof(player));
    memset(c, 0, sizeof(client));
    memset(p, 0, sizeof(player));
    
    /* initialize client*/
    c->socket_fd = fd;
    c->player = p;

    /* Init player */
    p->id = fd;
    p->team_id = team_id;
    p->level = 1;
    m_game_get_start_pos(&p->pos.x, &p->pos.y, &p->dir);
    
    /**/
    t_api = time_api_get_local();
    /*inventroy already 0*/
    p->die_time = t_api->current_time_units + TIME_TO_DIE; /* 1260 time units = 1 minute */


    /* add player to team */
    m_team_add_player_to_team(p);

    /* add player to server */
    m_add_client_to_server(c);

    /* add player to map */
    /* TODO */

    return SUCCESS;
}

int game_execute_command(int fd, char *cmd, char *arg)
{
    client *c;
    int ret;
    int i;
    command_type command;

    ret = m_game_get_client_from_fd(fd, &c);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to get client from fd %d\n", fd);
        return ERROR;
    }

    command = MAX_COMMANDS;
    for (i = 0; i < MAX_COMMANDS; i++)
    {
        if (strcmp(cmd, command_messages->name) == 0)
        {
            command = command_messages[i].type;
            break;
        }
    }

    if (command == MAX_COMMANDS)
    {
        fprintf(stderr, "Unknown command %s\n", cmd);
        return ERROR;
    }

    fprintf(stderr, "scheduling command %s\n", command_messages[command].name);
    fprintf(stderr, "Command %s with arg %s\n", command_messages[command].name, arg);
    fprintf(stderr, "Command %s with delay %d\n", command_messages[command].name, command_prototypes[command].delay);
    ret = time_api_schedule_client_event(NULL, &c->event_buffer,\
     command_prototypes[command].delay,\
     command_prototypes[command].prototype, c->player, arg);


    return SUCCESS;
}

void game_player_die(client *c)
{
    int ret;

    ret = m_remove_client_from_server(c);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove client from server\n");
        return;
    }
    ret = m_team_remove_player_from_team(c->player);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove player from team\n");
        return;
    }

    // ret = m_remove_player_from_map(c->player);

    free(c->player);

    server_create_response_to_command(c->socket_fd, "-", "die", NULL);
    fprintf(stderr, "Player %d has died\n", c->socket_fd);
    ret = server_remove_client(c->socket_fd);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove client from server\n");
        return;
    }

    free(c);

}

int game_play()
{
    int i;
    time_api* t_api;
    client* c;
    bool has_played;

    t_api = time_api_get_local();
    i = 0;
    has_played = false;
    while (i < m_server.client_count)
    {
        c = m_server.clients[i];
        i++;

        /**/
        if (c == NULL)
            continue; /* No client */

        if (!PLAYER_IS_ALIVE(c, t_api->current_time_units))
        {    
            game_player_die(c); /* Player not ready */
            continue;
        }

        if (CLIENT_HAS_ACTIONS(c, t_api->current_time_units))
        {
            time_api_process_client_events(NULL, &c->event_buffer);
            has_played = true;
        }       
    }


    /* check if players can play and then make them play */
    if (!has_played)
        return 0;
    return SUCCESS;
}
