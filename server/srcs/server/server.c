#include <stdio.h>
#include <cJSON.h>
#include <sys/select.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include <errno.h>
#include <ft_list.h>
#include "ssl_table.h"
#include "ssl_al.h"
#include "../game/game.h"
#include "../log/log.h"

/* ------------------------------------------------------------------ */
/*  Defines                                                            */
/* ------------------------------------------------------------------ */

#define SERVER_KEY      "SOME_KEY"
#define MAX_LOGIN_ROLES 3

/*
 * REMOVE_CLIENT – logs then tears down the fd.
 * game_kill_player is called first so the game layer can do a clean
 * "die" notification before the socket actually closes.
 */
#define REMOVE_CLIENT(fd)                                                   \
    do {                                                                    \
        FD_CLR(fd, &m_read_fds);                                           \
        log_net_disconnect((fd), "server-initiated remove");               \
        game_kill_player(fd);                                               \
        close(fd);                                                          \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Types                                                              */
/* ------------------------------------------------------------------ */

typedef enum
{
    type_cmd = 0,
    type_login,
    type_unknown
} client_message_type;

typedef struct
{
    client_message_type type;
    const char         *name;
} client_message;

typedef struct
{
    list_item_t item;
    int         fd;
} client_list_t;

typedef int (*client_message_handler)(int fd, cJSON *root);
typedef int (*login_handler)(int fd, cJSON *root);

/* ------------------------------------------------------------------ */
/*  Forward declarations                                               */
/* ------------------------------------------------------------------ */

static int m_handle_login(int fd, cJSON *root);
static int m_handle_cmd(int fd, cJSON *root);

static int m_handle_login_client(int fd, cJSON *root);
static int m_handle_login_admin(int fd, cJSON *root);
static int m_handle_login_observer(int fd, cJSON *root);

/* ------------------------------------------------------------------ */
/*  Dispatch tables                                                    */
/* ------------------------------------------------------------------ */

const client_message client_messages[] =
{
    {type_cmd,     "cmd"},
    {type_login,   "login"},
    {type_unknown, "unknown"}
};

static client_message_handler m_handlers[type_unknown] =
{
    m_handle_cmd,
    m_handle_login,
};

static const char *login_roles[MAX_LOGIN_ROLES] =
{
    "player",
    "admin",
    "observer"
};

static login_handler m_login_handlers[MAX_LOGIN_ROLES] =
{
    m_handle_login_client,
    m_handle_login_admin,
    m_handle_login_observer
};

/* ------------------------------------------------------------------ */
/*  State                                                              */
/* ------------------------------------------------------------------ */

static int          m_sock_server = -1;
static int          m_max_fd      = -1;
static fd_set       m_read_fds;
static client_list_t *m_clients   = NULL;

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static client_message_type m_get_message_type(const char *str)
{
    size_t i;

    for (i = 0; i < sizeof(client_messages) / sizeof(client_messages[0]) - 1; ++i)
    {
        if (strcmp(client_messages[i].name, str) == 0)
            return client_messages[i].type;
    }
    return type_unknown;
}

/* ------------------------------------------------------------------ */
/*  Send helpers                                                       */
/* ------------------------------------------------------------------ */

int server_send_json(int fd, void *resp)
{
    cJSON *json = (cJSON *)resp;
    char  *out  = cJSON_PrintUnformatted(json);

    log_net_send(fd, out);
    send(fd, out, strlen(out), 0);
    free(out);
    return SUCCESS;
}

int server_send(int fd, char *msg)
{
    log_net_send(fd, msg);
    if (send(fd, msg, strlen(msg), 0) == -1)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] send() failed for fd=%d: %s\n", fd, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Observer notification                                              */
/* ------------------------------------------------------------------ */

void m_server_notify_observers(int fd, cJSON *notification)
{
    int       i;
    char     *msg;
    observer **observers;
    observer  *o;

    observers = game_get_observers();
    if (!observers)
        return;

    cJSON_AddNumberToObject(notification, "player_id", fd);
    msg = cJSON_PrintUnformatted(notification);
    if (!msg)
        return;

    for (i = 0; observers[i]; i++)
    {
        o = observers[i];
        if (!o) continue;
        send(o->socket_fd, msg, strlen(msg), 0);
        log_msg(LOG_LEVEL_TRACE,
            "[SERVER][OBS_NOTIFY] observer_fd=%d player_fd=%d\n",
            o->socket_fd, fd);
    }
    free(msg);
}

/* ------------------------------------------------------------------ */
/*  JSON response builders                                             */
/* ------------------------------------------------------------------ */

int server_create_response_msg(int fd, char *cmd, char *arg, char *status)
{
    cJSON *response;
    char  *json;

    response = cJSON_CreateObject();
    if (!response) return ERROR;

    cJSON_AddStringToObject(response, "type", cmd);
    if (arg)    cJSON_AddStringToObject(response, "arg",    arg);
    if (status) cJSON_AddStringToObject(response, "status", status);

    json = cJSON_PrintUnformatted(response);
    if (!json) { cJSON_Delete(response); return ERROR; }

    log_net_send(fd, json);
    send(fd, json, strlen(json), 0);

    cJSON_Delete(response);
    free(json);
    return SUCCESS;
}

int server_create_response_to_command(int fd, char *cmd, char *arg, char *status)
{
    cJSON *response;
    char  *json;

    response = cJSON_CreateObject();
    if (!response) return ERROR;

    cJSON_AddStringToObject(response, "type", "response");
    cJSON_AddStringToObject(response, "cmd",  cmd);
    if (arg)    cJSON_AddStringToObject(response, "arg",    arg);
    if (status) cJSON_AddStringToObject(response, "status", status);

    json = cJSON_PrintUnformatted(response);
    if (!json) { cJSON_Delete(response); return ERROR; }

    log_cmd_executed(fd, cmd, status);
    log_net_send(fd, json);
    send(fd, json, strlen(json), 0);

    m_server_notify_observers(fd, response);

    cJSON_Delete(response);
    free(json);
    return SUCCESS;
}

static int m_create_json_response(int fd, char *type, char *msg, char *args)
{
    cJSON *response;
    char  *json;

    response = cJSON_CreateObject();
    if (!response) return ERROR;

    cJSON_AddStringToObject(response, "type", type);
    cJSON_AddStringToObject(response, "msg",  msg);
    if (args) cJSON_AddStringToObject(response, "args", args);

    json = cJSON_PrintUnformatted(response);
    if (!json) { cJSON_Delete(response); return ERROR; }

    log_net_send(fd, json);
    send(fd, json, strlen(json), 0);

    m_server_notify_observers(fd, response);

    cJSON_Delete(response);
    free(json);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Accept                                                             */
/* ------------------------------------------------------------------ */

int cb_on_accept_success(int fd)
{
    int ret;

    log_msg(LOG_LEVEL_INFO,
        "[SERVER][ACCEPT] New connection accepted: fd=%d\n", fd);

    FD_SET(fd, &m_read_fds);
    if (fd > m_max_fd)
        m_max_fd = fd;

    ret = m_create_json_response(fd, "bienvenue", "Knock knock, who's there?", NULL);
    if (ret == ERROR)
    {
        log_msg(LOG_LEVEL_WARN,
            "[SERVER][ACCEPT] fd=%d — failed to send bienvenue\n", fd);
        return ERROR;
    }
    return SUCCESS;
}

static int m_handle_new_client(int fd)
{
    int new_client;

    new_client = accept(fd, NULL, NULL);
    if (new_client == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] accept() failed: %s\n", strerror(errno));
        return ERROR;
    }

#ifndef USE_SSL
    cb_on_accept_success(new_client);
#endif

    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Message handlers                                                   */
/* ------------------------------------------------------------------ */

static int m_handle_cmd(int fd, cJSON *root)
{
    cJSON *key_value;
    cJSON *arg;
    int    ret;

    key_value = cJSON_GetObjectItem(root, "cmd");
    if (!key_value || !cJSON_IsString(key_value))
    {
        log_cmd_rejected(fd, "?", "missing 'cmd' field");
        return ERROR;
    }

    arg = cJSON_GetObjectItem(root, "arg");

    log_cmd_received(fd, key_value->valuestring,
        (arg && cJSON_IsString(arg)) ? arg->valuestring : NULL);

    if (arg && cJSON_IsString(arg))
        ret = game_execute_command(fd, key_value->valuestring, arg->valuestring);
    else
        ret = game_execute_command(fd, key_value->valuestring, NULL);

    return ret;
}

static int m_handle_login_admin(int fd, cJSON *root)
{
    (void)root;
    log_auth_fail(fd, "admin login not implemented");
    m_create_json_response(fd, "ko", "Not handled yet", NULL);
    return ERROR;
}

static int m_handle_login_observer(int fd, cJSON *root)
{
    int ret;

    (void)root;
    log_auth_attempt(fd, "observer", NULL);

    ret = game_register_observer(fd);
    if (ret == ERROR)
    {
        log_auth_fail(fd, "game_register_observer failed");
        m_create_json_response(fd, "error", "Failed to register observer", NULL);
        return ERROR;
    }

    log_auth_ok(fd, "observer", NULL);
    log_observer_connect(fd);
    m_create_json_response(fd, "ok", "Observer registered", NULL);
    return ret;
}

static int m_handle_login_client(int fd, cJSON *root)
{
    cJSON  *response;
    cJSON  *key_value;
    cJSON  *map_size;
    int     map_x;
    int     map_y;
    int     ret;
    char   *json;

    key_value = cJSON_GetObjectItem(root, "team-name");
    if (!key_value || !cJSON_IsString(key_value))
    {
        log_auth_fail(fd, "missing 'team-name' in login payload");
        m_create_json_response(fd, "error", "Invalid team name", NULL);
        return ERROR;
    }

    log_auth_attempt(fd, "player", key_value->valuestring);

    ret = game_register_player(fd, key_value->valuestring);
    if (ret == ERROR)
    {
        log_auth_fail(fd, "game_register_player failed (team full or unknown?)");
        m_create_json_response(fd, "error", "Failed to register player", NULL);
        return ERROR;
    }

    log_auth_ok(fd, "player", key_value->valuestring);

    response = cJSON_CreateObject();
    if (!response) return ERROR;

    cJSON_AddStringToObject(response, "type", "welcome");
    cJSON_AddNumberToObject(response, "remaining_clients",
        game_get_team_remaining_clients(fd));

    map_size = cJSON_CreateObject();
    if (!map_size) { cJSON_Delete(response); return ERROR; }

    game_get_map_size(&map_x, &map_y);
    cJSON_AddNumberToObject(map_size, "x", map_x);
    cJSON_AddNumberToObject(map_size, "y", map_y);
    cJSON_AddItemToObject(response, "map_size", map_size);

    int player_x, player_y, player_orientation;
    game_get_player_spawn(fd, &player_x, &player_y, &player_orientation);

    cJSON_AddNumberToObject(response, "x", player_x);
    cJSON_AddNumberToObject(response, "y", player_y);
    cJSON_AddNumberToObject(response, "orientation", player_orientation);

    json = cJSON_PrintUnformatted(response);
    if (!json) { cJSON_Delete(response); return ERROR; }

    log_net_send(fd, json);
    send(fd, json, strlen(json), 0);

    free(json);
    cJSON_Delete(response);
    return ret;
}

static int m_handle_login(int fd, cJSON *root)
{
    cJSON *key_value;
    int    i;
    int    ret;

    /* Validate server key */
    key_value = cJSON_GetObjectItem(root, "key");
    if (!key_value || !cJSON_IsString(key_value))
    {
        log_auth_fail(fd, "missing 'key' field");
        return ERROR;
    }
    if (strcmp(key_value->valuestring, SERVER_KEY) != 0)
    {
        log_auth_fail(fd, "wrong server key");
        m_create_json_response(fd, "error", "Invalid key", NULL);
        return ERROR;
    }

    /* Validate role */
    key_value = cJSON_GetObjectItem(root, "role");
    if (!key_value || !cJSON_IsString(key_value))
    {
        log_auth_fail(fd, "missing 'role' field");
        m_create_json_response(fd, "error", "Invalid team name", NULL);
        return ERROR;
    }

    for (i = 0; i < MAX_LOGIN_ROLES; ++i)
    {
        if (strcmp(key_value->valuestring, login_roles[i]) == 0)
            break;
    }
    if (i == MAX_LOGIN_ROLES)
    {
        log_auth_fail(fd, "unknown role");
        m_create_json_response(fd, "error", "Invalid role", NULL);
        return ERROR;
    }

    ret = m_login_handlers[i](fd, root);
    return ret;
}

/* ------------------------------------------------------------------ */
/*  Per-client event                                                   */
/* ------------------------------------------------------------------ */

static int m_handle_client_message(int fd, char *buffer, int bytes)
{
    client_message_type type;
    cJSON              *key_value;
    cJSON              *root;
    int                 ret;

    buffer[bytes] = '\0';

    log_net_recv(fd, buffer);

    root = cJSON_Parse(buffer);
    if (!root)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] fd=%d — JSON parse error near '%s'\n",
            fd, cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "?");
        log_msg(LOG_LEVEL_DEBUG,
            "[SERVER] fd=%d — raw buffer: %s\n", fd, buffer);
        return ERROR;
    }

    key_value = cJSON_GetObjectItem(root, "type");
    if (!key_value || !cJSON_IsString(key_value))
    {
        cJSON_Delete(root);
        m_create_json_response(fd, "error", "Invalid JSON format", NULL);
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] fd=%d — missing 'type' field in: %s\n", fd, buffer);
        return ERROR;
    }

    type = m_get_message_type(key_value->valuestring);
    if (type == type_unknown)
    {
        m_create_json_response(fd, "error", "Unknown message type", NULL);
        log_msg(LOG_LEVEL_WARN,
            "[SERVER] fd=%d — unknown message type='%s'\n",
            fd, key_value->valuestring);
        cJSON_Delete(root);
        return ERROR;
    }

    ret = m_handlers[type](fd, root);
    if (ret == ERROR)
    {
        /* The specific handler already sent an error response */
        log_msg(LOG_LEVEL_DEBUG,
            "[SERVER] fd=%d — handler for type='%s' returned ERROR\n",
            fd, key_value->valuestring);
        cJSON_Delete(root);
        return ERROR;
    }

    cJSON_Delete(root);
    return SUCCESS;
}

static int m_handle_client_event(int fd)
{
    char buffer[4096];
    int  bytes;
    int  ret;

    bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        if (bytes == -2)
            return SUCCESS; /* would-block from WS layer */

        if (bytes == -69)
        {
            log_msg(LOG_LEVEL_TRACE,
                "[SERVER] fd=%d — ping handled by WS layer\n", fd);
            return SUCCESS;
        }

        if (bytes == 0)
            log_net_disconnect(fd, "clean close by peer");
        else
            log_net_disconnect(fd, "recv error");

        client *c;
        if (m_game_get_client_from_fd(fd, &c) == SUCCESS && c && c->player) {
            log_msg(LOG_LEVEL_INFO, "Client %d disconnecting, clearing leadership flags\n", fd);
            if (c->player->is_leader) {
                m_team_clear_leader_flag(c->player->team_id, c->player->leader_level);
                c->player->is_leader = 0;
            }
        }

        REMOVE_CLIENT(fd);
        return SUCCESS;
    }

    ret = m_handle_client_message(fd, buffer, bytes);
    if (ret == ERROR)
    {
        /*
         * A bad message does NOT disconnect the client automatically —
         * the handler already sent an error JSON.  Return SUCCESS so
         * server_select keeps running.
         */
        log_msg(LOG_LEVEL_DEBUG,
            "[SERVER] fd=%d — bad message, client kept connected\n", fd);
    }
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Deferred client removal                                            */
/* ------------------------------------------------------------------ */

static void m_remove_clients(void)
{
    client_list_t *client;
    client_list_t *next_client;

    client = FT_LIST_GET_FIRST(&m_clients);
    while (client)
    {
        next_client = FT_LIST_GET_NEXT(&m_clients, client);
        REMOVE_CLIENT(client->fd);
        free(client);
        client = next_client;
    }
    m_clients = NULL;
}

int server_remove_client(int fd)
{
    client_list_t *client;

    if (fd < 0 || fd > m_max_fd)
        return ERROR;

    client = malloc(sizeof(client_list_t));
    if (!client) return ERROR;

    client->fd = fd;
    FT_LIST_ADD_LAST(&m_clients, client);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  server_select                                                      */
/* ------------------------------------------------------------------ */

int server_select(void)
{
    fd_set         read_fds;
    int            ret;
    int            fd;
    struct timeval timeout;

    ssl_al_lookup_new_clients();

    memcpy(&read_fds, &m_read_fds, sizeof(m_read_fds));

    timeout.tv_sec  = 0;
    timeout.tv_usec = 1000; /* 1 ms */

    ret = select(m_max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret < 0)
    {
        if (errno == EINTR) return 0;
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] select() error: ret=%d max_fd=%d errno=%d\n",
            ret, m_max_fd, errno);
        return ERROR;
    }
    if (ret == 0)
        return 0; /* timeout — nothing to do */

    for (fd = 0; fd <= m_max_fd; ++fd)
    {
        if (!FD_ISSET(fd, &read_fds))
            continue;

        if (fd == m_sock_server)
        {
            if (m_handle_new_client(fd) == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR,
                    "[SERVER] Failed to accept new connection\n");
            }
        }
        else
        {
            if (m_handle_client_event(fd) == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR,
                    "[SERVER] Unrecoverable error for fd=%d — removing\n", fd);
                REMOVE_CLIENT(fd);
            }
        }
    }

    m_remove_clients();
    return ret;
}

/* ------------------------------------------------------------------ */
/*  Init / cleanup                                                     */
/* ------------------------------------------------------------------ */

int init_server(int port, char *cert, char *key)
{
    m_sock_server = init_ssl_al(cert, key, port, cb_on_accept_success);
    if (m_sock_server == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SERVER] init_ssl_al() failed — cannot start server\n");
        return ERROR;
    }

    set_server_socket(m_sock_server);
    m_max_fd = m_sock_server;
    FD_ZERO(&m_read_fds);
    FD_SET(m_sock_server, &m_read_fds);

    log_msg(LOG_LEVEL_BOOT,
        "[SERVER] Listening socket ready: fd=%d port=%d\n",
        m_sock_server, port);
    return SUCCESS;
}

void cleanup_server(void)
{
    int fd;

    for (fd = 0; fd <= m_max_fd; ++fd)
    {
        if (FD_ISSET(fd, &m_read_fds))
            REMOVE_CLIENT(fd);
    }
    cleanup_ssl_al();
    ssl_table_free();
    log_msg(LOG_LEVEL_INFO, "[SERVER] Cleaned up\n");
}
