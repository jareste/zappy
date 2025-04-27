#include <ft_malloc.h>
#include <error_codes.h>
#include <ft_list.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <cJSON.h>
#include "game_structs.h"
#include "../time_api/time_api.h"
#include "../server/server.h"/* not liking it*/

#define PLAYER_IS_ALIVE(x, current_time) ((x->player->die_time > current_time))
#define CLIENT_HAS_ACTIONS(x, current_time) \
    ((x->event_buffer.count > 0) && (x->event_buffer.events[x->event_buffer.head].exec_time <= current_time))

#define TEAM_IS_FULL(id) (m_server.teams[id].current_players >= m_server.teams[id].max_players)

#define LIFE_UNIT 126 /* Stated in the subject */
#define TIME_TO_DIE (LIFE_UNIT*10) /* Stated in the subject */

#define MAP(x,y) (&(m_server.map[((y) * (m_server.map_x)) + (x)]))

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

typedef enum
{
    NOURRITURE = 0,
    LINEMATE,
    DERAUMERE,
    SIBUR,
    MENDIANE,
    PHIRAS,
    THYSTAME,
    UNKNOWN
} inventory_type;

typedef struct
{
    inventory_type type;
    char* name;
} inventory_strings;

static int m_command_avance(void* _p, void* _arg);
static int m_command_droite(void* _p, void* _arg);
static int m_command_gauche(void* _p, void* _arg);
static int m_command_connect_nbr(void* _p, void* _arg);
static int m_command_voir(void* _p, void* _arg);
static int m_command_inventaire(void* _p, void* _arg);
static int m_command_prend(void* _p, void* _arg);
static int m_command_pose(void* _p, void* _arg);
static int m_command_expulse(void* _p, void* _arg);
static int m_command_broadcast(void* _p, void* _arg);
static int m_command_incantation(void* _p, void* _arg);
static int m_command_fork(void* _p, void* _arg);

static server m_server = {0};

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
    {INCARNATION, "incantation"},
    {FORK, "fork"},
    {CONNECT_NBR, "connect_nbr"}
};

command command_prototypes[MAX_COMMANDS] =
{
    {m_command_avance, 7},
    {m_command_droite, 7},
    {m_command_gauche, 7},
    {m_command_voir, 7},
    {m_command_inventaire, 1},
    {m_command_prend, 7},
    {m_command_pose, 7},
    {m_command_expulse, 7},
    {m_command_broadcast, 7},
    {m_command_incantation, 300},
    {m_command_fork, 42},
    {m_command_connect_nbr, 0},
};

inventory_strings inventory_names[] =
{
    {NOURRITURE, "nourriture"},
    {LINEMATE,   "linemate"},
    {DERAUMERE,  "deraumere"},
    {SIBUR,      "sibur"},
    {MENDIANE,   "mendiane"},
    {PHIRAS,     "phiras"},
    {THYSTAME,   "thystame"},
    {UNKNOWN,    NULL}
};

spawn_ctx m_ctx;

static const double DENSITY_NOURRITURE = 1.0;
static const double DENSITY_LINEMATE   = 0.02;
static const double DENSITY_DERAUMERE  = 0.02;
static const double DENSITY_SIBUR      = 0.04;
static const double DENSITY_MENDIANE   = 0.04;
static const double DENSITY_PHIRAS     = 0.04;
static const double DENSITY_THYSTAME   = 0.005;

static int m_game_init_team(team *team, char *name, int max_players)
{
    team->name = strdup(name);
    team->max_players = max_players;
    team->current_players = 0;
    team->players = malloc(sizeof(player*) * max_players);
    memset(team->players, 0, sizeof(player*) * max_players);
    return SUCCESS;
}

static int m_game_get_team_id(char *name)
{
    int i;

    for (i = 0; i < m_server.team_count; i++)
    {
        if (strcmp(m_server.teams[i].name, name) == 0)
            return i;
    }
    return ERROR;
}

static int m_game_get_start_pos(int *x, int *y, direction* dir)
{
    *x = rand() % m_server.map_x;
    *y = rand() % m_server.map_y;
    *dir = rand() % 4; /* 0 = north, 1 = east, 2 = south, 3 = west */
    return SUCCESS;
}

static inline int inventory_sum(const inventory* inv)
{
    return inv->nourriture + inv->linemate + inv->deraumere +
           inv->sibur + inv->mendiane + inv->phiras + inv->thystame;
}

static cJSON* m_serialize_inventory(const inventory* inv)
{
    cJSON *o;

    o = cJSON_CreateObject();
    cJSON_AddNumberToObject(o, "nourriture", inv->nourriture);
    cJSON_AddNumberToObject(o, "linemate", inv->linemate);
    cJSON_AddNumberToObject(o, "deraumere", inv->deraumere);
    cJSON_AddNumberToObject(o, "sibur", inv->sibur);
    cJSON_AddNumberToObject(o, "mendiane", inv->mendiane);
    cJSON_AddNumberToObject(o, "phiras", inv->phiras);
    cJSON_AddNumberToObject(o, "thystame", inv->thystame);
    return o;
}

static cJSON* m_serialize_tile(tile* t)
{
    cJSON *o;
    cJSON *parr;
    player* p;

    o = cJSON_CreateObject();
    cJSON_AddNumberToObject(o, "x", t->pos.x);
    cJSON_AddNumberToObject(o, "y", t->pos.y);
    cJSON_AddItemToObject(o, "resources", m_serialize_inventory(&t->items));

    parr = cJSON_AddArrayToObject(o, "players");
    for (p = t->players; p; p = p->next_on_tile)
    {
        cJSON_AddItemToArray(parr, cJSON_CreateNumber(p->id));
    }
    return o;
}

static cJSON* m_serialize_player(const player* p)
{
    cJSON* o;
    cJSON *pos;

    o = cJSON_CreateObject();
    cJSON_AddNumberToObject(o, "id", p->id);

    pos = cJSON_AddObjectToObject(o, "position");
    cJSON_AddNumberToObject(pos, "x", p->pos.x);
    cJSON_AddNumberToObject(pos, "y", p->pos.y);

    cJSON_AddNumberToObject(o, "orientation", p->dir);
    cJSON_AddNumberToObject(o, "level", p->level);
    cJSON_AddStringToObject(o, "team", m_server.teams[p->team_id].name);
    cJSON_AddItemToObject(o, "inventory", m_serialize_inventory(&p->inv));

    return o;
}

char* m_serialize_server(void)
{
    cJSON* root;
    cJSON* map;
    cJSON* tiles;
    cJSON* t;
    cJSON* players;
    cJSON* teams;
    cJSON* game;
    time_api* t_api;
    player* p;
    int y;
    int x;
    int i;
    char* json;

    t_api = time_api_get_local();

    root = cJSON_CreateObject();
    map = cJSON_AddObjectToObject(root, "map");
    cJSON_AddNumberToObject(map, "width",  m_server.map_x);
    cJSON_AddNumberToObject(map, "height", m_server.map_y);

    tiles = cJSON_AddArrayToObject(map, "tiles");
    for (y = 0; y < m_server.map_y; y++)
    {
      for (x = 0; x < m_server.map_x; x++)
      {
        cJSON_AddItemToArray(tiles, m_serialize_tile(MAP(x, y)));
      }
    }

    players = cJSON_AddArrayToObject(root, "players");
    for (i = 0; i < m_server.client_count; i++)
    {
        p = m_server.clients[i]->player;
        if (!p)
            continue;
        cJSON_AddItemToArray(players, m_serialize_player(p));
    }

    game = cJSON_AddObjectToObject(root, "game");
    cJSON_AddNumberToObject(game, "tick", t_api->t);
    cJSON_AddNumberToObject(game, "time_unit", t_api->current_time_units);

    teams = cJSON_AddArrayToObject(game, "teams");
    for (i = 0; i < m_server.team_count; i++)
    {
        t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "name", m_server.teams[i].name);
        cJSON_AddNumberToObject(t, "player_count", m_server.teams[i].current_players);
        cJSON_AddNumberToObject(t, "remaining_connections", m_server.teams[i].max_players - m_server.teams[i].current_players);
        cJSON_AddItemToArray(teams, t);
    }

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static int m_send_map_observer(void* _p, void* _arg)
{
    observer *obs;
    char *json;

    (void)_arg;
    obs = (observer*)_p;
    json = m_serialize_server();
    server_send_json(obs->socket_fd, json);
    free(json);
    return SUCCESS;
}

// static void m_print_map()
// {
//     int i;
//     int j;
//     tile *t;

//     for (i = 0; i < m_server.map_x; i++)
//     {
//         for (j = 0; j < m_server.map_y; j++)
//         {
//             t = MAP(i, j);
//             printf("Tile (%d,%d): ", i, j);
//             if (t->players)
//                 printf("Players: %d\n", t->players->id);
//             else
//                 printf("No players\n");
//             printf("Items: ");
//             printf("Nourriture: %d, Linemate: %d, Deraumere: %d, Sibur: %d, Mendiane: %d, Phiras: %d, Thystame: %d\n",
//                    t->items.nourriture, t->items.linemate, t->items.deraumere,
//                    t->items.sibur, t->items.mendiane, t->items.phiras, t->items.thystame);
//         }
//     }
// }

int m_game_add_player_to_tile(tile *t, player *p)
{
    p->next_on_tile = t->players;
    p->prev_on_tile = NULL;
    if (t->players)
        t->players->prev_on_tile = p;
    t->players = p;

    p->pos = t->pos;
    return SUCCESS;
}

static void m_game_remove_player_from_tile(player *p)
{
    tile* t;
    
    t = MAP(p->pos.x, p->pos.y);
    if (p->prev_on_tile)
        p->prev_on_tile->next_on_tile = p->next_on_tile;
    else
        t->players = p->next_on_tile;

    if (p->next_on_tile)
        p->next_on_tile->prev_on_tile = p->prev_on_tile;

    p->next_on_tile = p->prev_on_tile = NULL;
}

static void m_game_move_player(player *p, int new_x, int new_y)
{
    m_game_remove_player_from_tile(p);

    p->pos.x = new_x;
    p->pos.y = new_y;

    m_game_add_player_to_tile(MAP(new_x, new_y), p);
}

static int m_game_get_client_from_fd(int fd, client **c)
{
    int i;

    if (!m_server.clients)
        return ERROR;

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

static int m_team_add_player_to_team(player *p)
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

static int m_team_remove_player_from_team(player *p)
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

static int m_add_client_to_server(client *c)
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

static int m_remove_client_from_server(client *c)
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

static inline int wrap(int v, int m)
{
    int r = v % m;
    return r < 0 ? r + m : r;
}

static cJSON* build_tile_vision(tile *T, player *p)
{
    cJSON *tile_arr;
    // player* p;
    // cJSON *obj;
    // int i;

    tile_arr = cJSON_CreateArray();

    // for (p = T->players; p; p = p->next_on_tile)
    // {
    //     obj = cJSON_CreateObject();
    //     cJSON_AddStringToObject(obj, "type",  "player");
    //     cJSON_AddStringToObject(obj, "team",  m_server.teams[p->team_id].name);
    //     cJSON_AddItemToArray(tile_arr, obj);
    // }

    if (T->players && T->players->id != p->id && p->next_on_tile != NULL)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("player"));

    // for (i = 0; i < T->items.nourriture; i++)
    if (T->items.nourriture > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("nourriture"));
    // for (i = 0; i < T->items.linemate; i++)
    if (T->items.linemate > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("linemate"));
    // for (i = 0; i < T->items.deraumere; i++)
    if (T->items.deraumere > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("deraumere"));
    // for (i = 0; i < T->items.sibur; i++)
    if (T->items.sibur > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("sibur"));
    // for (i = 0; i < T->items.mendiane; i++)
    if (T->items.mendiane > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("mendiane"));
    // for (i = 0; i < T->items.phiras; i++)
    if (T->items.phiras > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("phiras"));
    // for (i = 0; i < T->items.thystame; i++)
    if (T->items.thystame > 0)
        cJSON_AddItemToArray(tile_arr, cJSON_CreateString("thystame"));
    return tile_arr;
}

int m_command_voir(void* _p, void* _arg)
{
    player *p;
    tile* T;
    int width;
    int rel_x;
    int rel_y;
    int x;
    int y;
    int d;
    int i;
    int lvl;
    cJSON* root;
    cJSON* vision;
    cJSON* tile_arr;

    (void)_arg;

    p = (player*)_p;

    lvl = p->level;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response");
    cJSON_AddStringToObject(root, "cmd", "voir");

    vision = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "vision", vision);

    /* DEBUG */
    // fprintf(stderr, "Player %d t(%d,%d) dir(", p->id, p->pos.x, p->pos.y);
    // switch (p->dir)
    // {
    //     case NORTH: fprintf(stderr, "N"); break;
    //     case EAST:  fprintf(stderr, "E");  break;
    //     case SOUTH: fprintf(stderr, "S"); break;
    //     case WEST:  fprintf(stderr, "W");  break;
    // }
    // fprintf(stderr, ") sees:\n");
    /* DEBUG_END */

    for (d = 0; d <= lvl; d++)
    {
        width = 2*d + 1;
        for (i = 0; i < width; i++)
        {
            switch (p->dir)
            {
                case NORTH:
                    rel_x = p->pos.x - d + i;
                    // rel_y = p->pos.y - (d + 1);
                    rel_y = p->pos.y - d;
                    break;
                case EAST:
                    // rel_x = p->pos.x + (d + 1);
                    rel_x = p->pos.x + d;
                    rel_y = p->pos.y - d + i;
                    break;
                case SOUTH:
                    rel_x = p->pos.x + d - i;
                    rel_y = p->pos.y + d;
                    // rel_y = p->pos.y + (d + 1);
                    break;
                case WEST:
                    rel_x = p->pos.x - d;
                    // rel_x = p->pos.x - (d + 1);
                    rel_y = p->pos.y + d - i;
                    break;
                default:
                    rel_x = p->pos.x;
                    rel_y = p->pos.y;
            }
            // fprintf(stderr, "rel_x: %d, rel_y: %d\n", rel_x, rel_y);
            x = wrap(rel_x, m_server.map_x);
            y = wrap(rel_y, m_server.map_y);
            // fprintf(stderr, "x: %d, y: %d\n", x, y);
            T = MAP(x, y);

            tile_arr = build_tile_vision(T, p);
            cJSON_AddItemToArray(vision, tile_arr);
        }
    }

    server_send_json(p->id, root);
    /* DEBUG */
    // char* json = cJSON_Print(root);
    // fprintf(stderr, "Player %d t(%d,%d) dir(", p->id, p->pos.x, p->pos.y);
    // switch (p->dir)
    // {
    //     case NORTH: fprintf(stderr, "N"); break;
    //     case EAST:  fprintf(stderr, "E");  break;
    //     case SOUTH: fprintf(stderr, "S"); break;
    //     case WEST:  fprintf(stderr, "W");  break;
    // }
    // fprintf(stderr, ") sees:\n%s\n", json);
    /* DEBUG_END */

    cJSON_Delete(root);

    return SUCCESS;
}

static int m_command_inventaire(void* _p, void* _arg)
{
    player *p;
    cJSON* root;
    cJSON* inv;

    (void)_arg;

    p = (player*)_p;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response");
    cJSON_AddStringToObject(root, "cmd", "inventaire");

    inv = cJSON_CreateObject();
    cJSON_AddNumberToObject(inv, "nourriture", p->inv.nourriture);
    cJSON_AddNumberToObject(inv, "linemate",   p->inv.linemate);
    cJSON_AddNumberToObject(inv, "deraumere",  p->inv.deraumere);
    cJSON_AddNumberToObject(inv, "sibur",      p->inv.sibur);
    cJSON_AddNumberToObject(inv, "mendiane",   p->inv.mendiane);
    cJSON_AddNumberToObject(inv, "phiras",     p->inv.phiras);
    cJSON_AddNumberToObject(inv, "thystame",   p->inv.thystame);

    cJSON_AddItemToObject(root, "inventaire", inv);

    server_send_json(p->id, root);
    cJSON_Delete(root);

    return SUCCESS;
}

static int m_helper_items_to_tiles(tile* t, player* p, int add, inventory_type type)
{
    switch (type)
    {
        case NOURRITURE:
            if (add == -1)
            {
                if (t->items.nourriture <= 0) return ERROR;
                t->items.nourriture--;
                p->inv.nourriture++;
            }
            else if (add == 1)
            {
                if (p->inv.nourriture <= 0) return ERROR;
                t->items.nourriture++;
                p->inv.nourriture--;
            }
            break;
        case LINEMATE:
            if (add == -1)
            {
                if (t->items.linemate <= 0) return ERROR;
                t->items.linemate--;
                p->inv.linemate++;
            }
            else if (add == 1)
            {
                if (p->inv.linemate <= 0) return ERROR;
                t->items.linemate++;
                p->inv.linemate--;
            }
            break;
        case DERAUMERE:
            if (add == -1)
            {
                if (t->items.deraumere <= 0) return ERROR;
                t->items.deraumere--;
                p->inv.deraumere++;
            }
            else if (add == 1)
            {
                if (p->inv.deraumere <= 0) return ERROR;
                t->items.deraumere++;
                p->inv.deraumere--;
            }
            break;
        case SIBUR:
            if (add == -1)
            {
                if (t->items.sibur <= 0) return ERROR;
                t->items.sibur--;
                p->inv.sibur++;
            }
            else if (add == 1)
            {
                if (p->inv.sibur <= 0) return ERROR;
                t->items.sibur++;
                p->inv.sibur--;
            }
            break;
        case MENDIANE:
            if (add == -1)
            {
                if (t->items.mendiane <= 0) return ERROR;
                t->items.mendiane--;
                p->inv.mendiane++;
            }
            else if (add == 1)
            {
                if (p->inv.mendiane <= 0) return ERROR;
                t->items.mendiane++;
                p->inv.mendiane--;
            }
            break;
        case PHIRAS:
            if (add == -1)
            {
                if (t->items.phiras <= 0) return ERROR;
                t->items.phiras--;
                p->inv.phiras++;
            }
            else if (add == 1)
            {
                if (p->inv.phiras <= 0) return ERROR;
                t->items.phiras++;
                p->inv.phiras--;
            }
            break;
        case THYSTAME:
            if (add == -1)
            {
                if (t->items.thystame <= 0) return ERROR;
                t->items.thystame--;
                p->inv.thystame++;
            }
            else if (add == 1)
            {
                if (p->inv.thystame <= 0) return ERROR;
                t->items.thystame++;
                p->inv.thystame--;
            }
            break;
        default:
            return ERROR;
    }
    return SUCCESS;
}

static int m_command_prend(void* _p, void* _arg)
{
    player* p;
    char* arg;
    inventory_type type;
    int i;
    int ret;

    p = (player*)_p;
    arg = (char*)_arg;

    if (!arg)
        return server_create_response_to_command(p->id, "prend", "Invalid arg.", "ko");

    type = UNKNOWN;
    for (i = 0; inventory_names[i].name; i++)
    {
        if (strcmp(arg, inventory_names[i].name) == 0)
        {
            type = inventory_names[i].type;
            break;
        }
    }

    if (type == UNKNOWN)
        return server_create_response_to_command(p->id, "prend", "Unknown type.", "ko");

    if (m_helper_items_to_tiles(MAP(p->pos.x, p->pos.y), p, -1, type) == ERROR)
        return server_create_response_to_command(p->id, "prend", arg, "ko");

    ret = server_create_response_to_command(p->id, "prend", arg, "ok");

    free(_arg);

    return ret;
}

static int m_command_pose(void* _p, void* _arg)
{
    player* p;
    char* arg;
    inventory_type type;
    int i;
    int ret;

    p = (player*)_p;
    arg = (char*)_arg;
    if (!arg)
        return server_create_response_to_command(p->id, "pose", "Invalid arg.", "ko");
    
    type = UNKNOWN;
    for (i = 0; inventory_names[i].name; i++)
    {
        if (strcmp(arg, inventory_names[i].name) == 0)
        {
            type = inventory_names[i].type;
            break;
        }
    }

    if (type == UNKNOWN)
        return server_create_response_to_command(p->id, "pose", "Unknown type.", "ko");

    if (m_helper_items_to_tiles(MAP(p->pos.x, p->pos.y), p, 1, type) == ERROR)
        return server_create_response_to_command(p->id, "pose", arg, "ko");

    ret = server_create_response_to_command(p->id, "pose", arg, "ok");

    free(_arg);

    return ret;
}

static int m_command_expulse(void* _p, void* _arg)
{
    player* p;
    player* it;
    tile* t;
    int new_x;
    int new_y;
    const char* dir_string;
    static const char* direction_table[4][4] = {
        /* NORTH */ {"1", "7", "5", "3"},
        /* EAST  */ {"3", "1", "7", "5"},
        /* SOUTH */ {"5", "3", "1", "7"},
        /* WEST  */ {"7", "5", "3", "1"}
                   /* N    E    S    W */
    };

    (void)_arg;
    p = (player*)_p;
    t = MAP(p->pos.x, p->pos.y);
    if (t->players == NULL)
        return server_create_response_to_command(p->id, "expulse", NULL, "ok");

    new_x = p->pos.x;
    new_y = p->pos.y;

    switch (p->dir)
    {
        case NORTH: new_y = (p->pos.y + m_server.map_y - 1) % m_server.map_y; break;
        case EAST: new_x = (p->pos.x + 1) % m_server.map_x; break;
        case SOUTH: new_y = (p->pos.y + 1) % m_server.map_y; break;
        case WEST: new_x = (p->pos.x + m_server.map_x - 1) % m_server.map_x; break;
    }

    for (it = t->players; it; it = it->next_on_tile)
    {
        if (it->id == p->id)
            continue;

        dir_string = direction_table[p->dir][p->dir];

        m_game_move_player(it, new_x, new_y);
        server_create_response_to_command(it->id, "deplacement", NULL, (char*)dir_string);
    }

    return server_create_response_to_command(p->id, "expulse", NULL, "ok");
}

static int m_command_broadcast(void* _p, void* _arg)
{
    player* p;
    (void)_arg;

    p = (player*)_p;

    return server_create_response_to_command(p->id, "broadcast", NULL, "ok");
}

static int m_command_incantation(void* _p, void* _arg)
{
    player* p;
    (void)_arg;

    p = (player*)_p;

    return server_create_response_to_command(p->id, "incantation", NULL, "ok");
}

static int m_command_fork(void* _p, void* _arg)
{
    player* p;
    (void)_arg;

    p = (player*)_p;

    return server_create_response_to_command(p->id, "fork", NULL, "ok");
}

static int m_command_connect_nbr(void* _p, void* _arg)
{
    player* p;
    char number[10];
    
    (void)_arg;

    p = (player*)_p;
    snprintf(number, sizeof(number),\
     "%d", m_server.teams[p->team_id].max_players - m_server.teams[p->team_id].current_players);

    return server_create_response_to_command(p->id, "connect_nbr", number,  NULL);
}

static int m_command_droite(void* _p, void* _arg)
{
    player *p;
    char   *arg;

    p = (player*)_p;
    arg = (char*)_arg;
    switch (p->dir)
    {
        case NORTH:
            p->dir = EAST;
            break;
        case EAST:
            p->dir = SOUTH;
            break;
        case SOUTH:
            p->dir = WEST;
            break;
        case WEST:
            p->dir = NORTH;
            break;
    }

    return server_create_response_to_command(p->id, "droite", arg, "ok");
}

static int m_command_gauche(void* _p, void* _arg)
{
    player *p;
    char   *arg;

    p = (player*)_p;
    arg = (char*)_arg;
    switch (p->dir)
    {
        case NORTH:
            p->dir = WEST;
            break;
        case EAST:
            p->dir = NORTH;
            break;
        case SOUTH:
            p->dir = EAST;
            break;
        case WEST:
            p->dir = SOUTH;
            break;
    }

    return server_create_response_to_command(p->id, "gauche", arg, "ok");
}

static int m_command_avance(void* _p, void* _arg)
{
    player* p;
    char* arg;
    int new_x;
    int new_y;

    p = (player*)_p;
    arg = (char*)_arg;
    new_x = p->pos.x;
    new_y = p->pos.y;
    switch (p->dir)
    {
      case NORTH: new_y = (p->pos.y + m_server.map_y - 1) % m_server.map_y; break;
      case EAST:  new_x = (p->pos.x + 1) % m_server.map_x;                   break;
      case SOUTH: new_y = (p->pos.y + 1) % m_server.map_y;                   break;
      case WEST:  new_x = (p->pos.x + m_server.map_x - 1) % m_server.map_x;  break;
    }

    /* DEBUG */
    // printf("Player %d moved from (%d,%d) to (%d,%d)\n", p->id, p->pos.x, p->pos.y, new_x, new_y);
    // printf("Player %d dir: %d(", p->id, p->dir);
    // switch (p->dir)
    // {
    //     case NORTH: printf("N)\n"); break;
    //     case EAST:  printf("E)\n"); break;
    //     case SOUTH: printf("S)\n"); break;
    //     case WEST:  printf("W)\n"); break;
    // }
    /* DEBUG_END */

    m_game_move_player(p, new_x, new_y);
 
    return server_create_response_to_command(p->id, "avance", arg, "ok");
}

static void m_game_print_players_on_tile(tile *t)
{
    player *it;

    printf("Players on tile (%d,%d):\n", t->pos.x, t->pos.y);
    for (it = t->players; it; it = it->next_on_tile)
    {
        printf(" - Player %d (team %d, lvl %d, dir %d)\n", it->id, it->team_id+1, it->level, it->dir);
    }
}

static int m_game_random_resource_count(double lambda)
{
    int count;
    double p0;
    double prod;

    p0 = exp(-lambda);
    prod = 1.0;
    count = 0;
    while (1)
    {
        prod *= (rand() / (double)RAND_MAX);
        if (prod < p0) break;
        count++;
    }
    return count;
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

int game_register_observer(int fd)
{
    observer* o;

    o = malloc(sizeof(observer));
    memset(o, 0, sizeof(observer));

    o->socket_fd = fd;

    time_api_schedule_client_event(NULL, &o->event_buffer,\
     0, m_send_map_observer, o, NULL);

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

    m_game_add_player_to_tile(MAP(p->pos.x, p->pos.y), p);

    /**/
    t_api = time_api_get_local();
    /*inventroy already 0*/
    p->die_time = t_api->current_time_units + TIME_TO_DIE; /* 1260 time units = 1 minute */
    p->start_time = t_api->current_time_units; /* 1260 time units = 1 minute */

    /* add player to team */
    m_team_add_player_to_team(p);

    /* add player to server */
    m_add_client_to_server(c);

    fprintf(stderr, "Spawned player %d on tile (%d,%d,%d) for team %s\n", p->id, p->pos.x, p->pos.y, p->dir,team_name);

    return SUCCESS;
}

int game_execute_command(int fd, char *cmd, char *_arg)
{
    client *c;
    int ret;
    int i;
    command_type command;
    char* arg;

    ret = m_game_get_client_from_fd(fd, &c);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to get client from fd %d\n", fd);
        return ERROR;
    }

    command = MAX_COMMANDS;
    for (i = 0; i < MAX_COMMANDS; i++)
    {
        if (strcmp(cmd, command_messages[i].name) == 0)
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

    if (_arg)
        arg = strdup(_arg);
    else
        arg = NULL;

    ret = time_api_schedule_client_event(NULL, &c->event_buffer,\
     command_prototypes[command].delay,\
     command_prototypes[command].prototype, c->player, arg);


    return SUCCESS;
}

int game_player_die(client *c)
{
    int ret;

    if (c->player->inv.nourriture > 0)
    {
        fprintf(stderr, "Player %d has eaten food\n", c->socket_fd);
        c->player->inv.nourriture--;
        c->player->die_time = c->player->die_time + LIFE_UNIT;
        return SUCCESS;
    }

    m_game_print_players_on_tile(MAP(c->player->pos.x, c->player->pos.y));
    fprintf(stderr, "Player %d has died. '%d', '%d'\n", c->socket_fd, c->player->die_time, c->player->start_time);

    fprintf(stderr, "Player inventory:\n");
    fprintf(stderr, " - nourriture: %d\n", c->player->inv.nourriture);
    fprintf(stderr, " - linemate:   %d\n", c->player->inv.linemate);
    fprintf(stderr, " - deraumere:  %d\n", c->player->inv.deraumere);
    fprintf(stderr, " - sibur:      %d\n", c->player->inv.sibur);
    fprintf(stderr, " - mendiane:   %d\n", c->player->inv.mendiane);
    fprintf(stderr, " - phiras:     %d\n", c->player->inv.phiras);
    fprintf(stderr, " - thystame:   %d\n", c->player->inv.thystame);
    fprintf(stderr, "Player position: (%d,%d)\n", c->player->pos.x, c->player->pos.y);

    ret = m_remove_client_from_server(c);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove client from server\n");
        return ERROR;
    }
    ret = m_team_remove_player_from_team(c->player);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove player from team\n");
        return ERROR;
    }

    m_game_remove_player_from_tile(c->player);
    free(c->player);

    server_create_response_to_command(c->socket_fd, "-", "die", NULL);
    fprintf(stderr, "Player %d has died\n", c->socket_fd);
    ret = server_remove_client(c->socket_fd);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to remove client from server\n");
        return ERROR;
    }

    free(c);
    return SUCCESS;
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
            game_player_die(c);
            continue;
        }

        if (CLIENT_HAS_ACTIONS(c, t_api->current_time_units))
        {
            time_api_process_client_events(NULL, &c->event_buffer);
            has_played = true;
        }       
    }

    time_api_process_client_events(NULL, &m_server.event_buffer);

    /* check if players can play and then make them play */
    if (!has_played)
        return 0;
    return SUCCESS;
}

int game_kill_player(int fd)
{
    client *c;
    int ret;

    ret = m_game_get_client_from_fd(fd, &c);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to get client from fd %d\n", fd);
        return ERROR;
    }

    if (c->player == NULL)
    {
        fprintf(stderr, "Player %d is already dead\n", fd);
        return ERROR;
    }

    c->player->die_time = 0;
    c->player->inv.nourriture = 0;

    return SUCCESS;
}

int m_game_spawn_resources(void* data, void* arg)
{
    const int W = m_server.map_x;
    const int H = m_server.map_y;
    const int MAP_SZ = W * H;
    int batch;
    int idx;
    int x;
    int y;
    tile* T;
    int i;

    batch = (MAP_SZ * 5 + 99) / 100;  
    if (batch < 1) batch = 1;

    if (batch > 1000) batch = 1000;

    for (i = 0; i < batch; i++)
    {
        idx = (m_ctx.next_idx + i) % MAP_SZ;
        x = idx % W;
        y = idx / W;
        T = MAP(x, y);

        if (inventory_sum(&T->items) > 15)
            continue;

        T->items.nourriture += m_game_random_resource_count(m_ctx.d_nourriture);
        T->items.linemate += m_game_random_resource_count(m_ctx.d_linemate);
        T->items.deraumere += m_game_random_resource_count(m_ctx.d_deraumere);
        T->items.sibur += m_game_random_resource_count(m_ctx.d_sibur);
        T->items.mendiane += m_game_random_resource_count(m_ctx.d_mendiane);
        T->items.phiras += m_game_random_resource_count(m_ctx.d_phiras);
        T->items.thystame += m_game_random_resource_count(m_ctx.d_thystame);
    }

    m_ctx.next_idx = (m_ctx.next_idx + batch) % MAP_SZ;

    time_api_schedule_client_event(
      NULL,
      &m_server.event_buffer,
      m_ctx.period,
      m_game_spawn_resources,
      data,
      arg
    );

    return 0;
}

int game_init_map(int width, int height)
{
    int i;
    int j;

    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            MAP(i, j)->pos.x = i;
            MAP(i, j)->pos.y = j;
            MAP(i, j)->players = NULL;

            MAP(i, j)->items.nourriture = m_game_random_resource_count(DENSITY_NOURRITURE);
            MAP(i, j)->items.linemate   = m_game_random_resource_count(DENSITY_LINEMATE);
            MAP(i, j)->items.deraumere  = m_game_random_resource_count(DENSITY_DERAUMERE);
            MAP(i, j)->items.sibur      = m_game_random_resource_count(DENSITY_SIBUR);
            MAP(i, j)->items.mendiane   = m_game_random_resource_count(DENSITY_MENDIANE);
            MAP(i, j)->items.phiras     = m_game_random_resource_count(DENSITY_PHIRAS);
            MAP(i, j)->items.thystame   = m_game_random_resource_count(DENSITY_THYSTAME);
        }
    }
    return SUCCESS;
}

void game_clean()
{
    int i;

    for (i = 0; i < m_server.client_count; i++)
    {
        if (m_server.clients[i])
        {
            free(m_server.clients[i]->player);
            free(m_server.clients[i]);
        }
    }
    for (i = 0; i < m_server.team_count; i++)
    {
        free(m_server.teams[i].players);
        free(m_server.teams[i].name);
    }
    free(m_server.clients);
    m_server.clients = NULL;
    free(m_server.teams);
    m_server.teams = NULL;
    free(m_server.map);
    m_server.map = NULL;
}

int game_init(int width, int height, char **teams, int nb_clients, int nb_teams)
{
    int team_number;
    int i;
    int ret;

    m_server.map_x = width;
    m_server.map_y = height;
    m_server.map = malloc(sizeof(tile) * width * height);
    memset(m_server.map, 0, sizeof(tile) * width * height);
    game_init_map(width, height);
    m_server.teams = malloc(sizeof(team) * nb_teams);
    memset(m_server.teams, 0, sizeof(team) * nb_teams);
    m_server.clients = malloc(sizeof(client*) * nb_clients);
    memset(m_server.clients, 0, sizeof(client*) * nb_clients);
    m_server.client_count = nb_clients;
    m_server.team_count = nb_teams;

    i = 0;
    team_number = 0;
    while (i < nb_teams)
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

    m_ctx.d_nourriture  = 0.5;
    m_ctx.d_linemate    = 0.02;
    m_ctx.d_deraumere   = 0.02;
    m_ctx.d_sibur       = 0.04;
    m_ctx.d_mendiane    = 0.04;
    m_ctx.d_phiras      = 0.04;
    m_ctx.d_thystame    = 0.005;
    m_ctx.period        = 100;
    m_ctx.next_idx      = 0;

    time_api_schedule_client_event(NULL, &m_server.event_buffer, m_ctx.period, m_game_spawn_resources, NULL, NULL);

    return SUCCESS;
}
