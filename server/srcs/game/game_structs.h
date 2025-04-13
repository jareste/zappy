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

typedef struct
{
    position pos;
    inventory items;
    /* needing to know players here, maybe a list would be ok? */
} tile;

typedef struct
{
    int id; /* Sockfd */
    int team_id;
    int level;
    position pos;
    direction dir;
    inventory inv;
    int die_time; /* life units (1260 time units) */
} player;

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
    void (*callback)(void *);
    void* data;
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
    char* name;
    int delay;
    void (*callback)(void *);
} command;

typedef struct
{
    int map_x;
    int map_y;
  
    tile** map;
    team* teams;
    int team_count;
    client** clients;
    int client_count;
} server;

#endif /* GAME_STRUCTS_H */
