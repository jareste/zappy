#ifndef GAME_STRUCTS_H
#define GAME_STRUCTS_H

#define MAX_EVENTS 10

typedef enum
{
    NORTH,
    EAST,
    SOUTH,
    WEST
} direction;

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

typedef struct player_s
{
    int id; /* Sockfd */
    int team_id;
    int level;
    position pos;
    direction dir;
    inventory inv;
    int die_time; /* life units (1260 time units) */
    int start_time; /* time when the player was created */

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
    int exec_time;
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
    char buffer[1024]; /* not sure if i need that */
    event_buffer event_buffer;
} client;

typedef struct
{
    int map_x;
    int map_y;
  
    tile* map;
    team* teams;
    int team_count;
    client** clients;
    int client_count;

    event_buffer event_buffer;
} server;

#endif /* GAME_STRUCTS_H */
