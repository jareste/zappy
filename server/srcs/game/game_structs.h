#ifndef GAME_STRUCTS_H
#define GAME_STRUCTS_H

#include <stdint.h>

#define MAX_EVENTS 10

typedef enum
{
    NORTH,
    EAST,
    SOUTH,
    WEST
} direction;

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
    INCANTATION,
    FORK,
    CONNECT_NBR,
    MAX_COMMANDS
} command_type;

typedef struct
{
    int x;
    int y;
} position;

typedef struct
{
    int nourriture;
    int linemate;
    int deraumere;
    int sibur;
    int mendiane;
    int phiras;
    int thystame;
} inventory;

typedef struct
{
    double  d_nourriture;
    double  d_linemate;
    double  d_deraumere;
    double  d_sibur;
    double  d_mendiane;
    double  d_phiras;
    double  d_thystame;
    int     period;
    int     next_idx;
} spawn_ctx;

typedef int (*command_prototype)(void* p, void* arg);

typedef struct 
{
    command_prototype prototype;
    int delay;
} command;

typedef struct player_s
{
    int id; /* Sockfd */
    int team_id;
    int level;
    position pos;
    direction dir;
    inventory inv;
    uint64_t die_time; /* life units (1260 time units) */
    uint64_t start_time; /* time when the player was created */

    struct player_s* next_on_tile;
    struct player_s* prev_on_tile;
} player;

typedef struct
{
    position pos;
    inventory items;

    player*     players;
} tile;

typedef struct
{
    char* name;
    int max_players;
    int current_players;
    int* players; /* index of the player in 'server.clients' */
} team;

typedef struct
{
    uint64_t exec_time;
    int (*callback)(void *, void *);
    void* data;
    void* arg;
} event;

typedef struct 
{
    event events[MAX_EVENTS];
    int count;
    int head;
    int tail;
} event_buffer;

typedef struct
{
    int socket_fd;
    player* player;
    event_buffer event_buffer;
} client;

typedef struct
{
    int socket_fd;
    int last_idx;
    event_buffer event_buffer;
} observer;

typedef struct
{
    int map_x;
    int map_y;
  
    tile* map;
    team* teams;
    int team_count;
    client** clients;
    int client_count;
    observer** observers;

    event_buffer event_buffer;
} server;

#endif /* GAME_STRUCTS_H */
