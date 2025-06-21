#include <stdio.h>
#include <cJSON.h>
#include <sys/select.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include <ft_list.h>
#include "ssl_table.h"
#include "ssl_al.h"
#include "../game/game.h"
#include "../log/log.h"

/* defines */
#define SERVER_KEY "SOME_KEY"
#define MAX_LOGIN_ROLES 3

#define REMOVE_CLIENT(fd) \
    do { \
        FD_CLR(fd, &m_read_fds); \
        log_msg(LOG_LEVEL_INFO, "Removing client %d\n", fd); \
        game_kill_player(fd); \
        close(fd); \
    } while (0)

/* DEBUG */
/*
static struct timeval m_start_time;
static long m_elapsed_us = 0;
static struct timeval m_end_time;
#define START_TIMER \
do { \
    gettimeofday(&m_start_time, NULL); \
} while (0)


#define END_TIMER \
do { \
    gettimeofday(&m_end_time, NULL); \
    m_elapsed_us = (m_end_time.tv_sec - m_start_time.tv_sec) * 1000000L + \
                      (m_end_time.tv_usec - m_start_time.tv_usec); \
    printf("Elapsed time: %ld microseconds\n", m_elapsed_us); \
} while (0)
*/
/* DEBUG_END */

/* Typedefs */
typedef enum
{
    type_cmd = 0,
    type_login,
    type_unknown
} client_message_type;

typedef struct
{
    client_message_type type;
    const char *name;
} client_message;

typedef struct
{
    list_item_t item;
    int fd;
} client_list_t;

typedef int (*client_message_handler)(int fd, cJSON *root);
typedef int (*login_handler)(int fd, cJSON *root);

/* Prototypes */
static int m_handle_login(int fd, cJSON *root);
static int m_handle_cmd(int fd, cJSON *root);

static int m_handle_login_client(int fd, cJSON *root);
static int m_handle_login_admin(int fd, cJSON *root);
static int m_handle_login_observer(int fd, cJSON *root);

/* Locals */
const client_message client_messages[] =
{
    {type_cmd, "cmd"},
    {type_login, "login"},
    {type_unknown, "unknown"}
};

static client_message_handler m_handlers[type_unknown] =
{
    m_handle_cmd, /* type_cmd */
    m_handle_login,
};

static const char* login_roles[MAX_LOGIN_ROLES] =
{
    "player",
    "admin",
    "observer"
};

static login_handler m_login_handlers[MAX_LOGIN_ROLES] =
{
    m_handle_login_client, /* player */
    m_handle_login_admin, /* admin */
    m_handle_login_observer  /* observer */
};

static int m_sock_server = -1;
static int m_max_fd = -1;
static fd_set m_read_fds;
static client_list_t* m_clients = NULL;

/* Definitions */
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

int server_send_json(int fd, void* resp)
{
    cJSON* json;
    char *out;

    json = (cJSON*)resp;
    out = cJSON_PrintUnformatted(json);
    send(fd, out, strlen(out), 0);
    free(out);
    return SUCCESS;
}

int server_create_response_to_command(int fd, char *cmd, char *arg, char* status)
{
    cJSON *response;
    char *json;

    response = cJSON_CreateObject();
    if (!response)
        return ERROR;

    cJSON_AddStringToObject(response, "type", "response");
    cJSON_AddStringToObject(response, "cmd", cmd);
    if (arg)
        cJSON_AddStringToObject(response, "arg", arg);
    if (status)
        cJSON_AddStringToObject(response, "status", status);

    json = cJSON_Print(response);
    if (!json)
    {
        cJSON_Delete(response);
        return ERROR;
    }

    send(fd, json, strlen(json), 0);

    cJSON_Delete(response);
    free(json);

    return SUCCESS;
}

static int m_create_json_response(int fd, char* type, char* msg, char* args)
{
    cJSON *response;
    char *json;

    response = cJSON_CreateObject();
    if (!response)
        return ERROR;

    cJSON_AddStringToObject(response, "type", type);
    cJSON_AddStringToObject(response, "msg", msg);
    if (args)
        cJSON_AddStringToObject(response, "args", args);

    json = cJSON_Print(response);
    if (!json)
    {
        cJSON_Delete(response);
        return ERROR;
    }

    send(fd, json, strlen(json), 0);

    cJSON_Delete(response);
    free(json);

    return SUCCESS;
}

int cb_on_accept_success(int fd)
{
    int ret;

    FD_SET(fd, &m_read_fds);
    if (fd > m_max_fd)
        m_max_fd = fd;

    /**/

    ret = m_create_json_response(fd, "bienvenue", "Whoa! Knock knock, whos there?", NULL);
    if (ret == ERROR)
    {
        log_msg(LOG_LEVEL_WARN, "Failed to create JSON response\n");
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
        perror("accept");
        return ERROR;
    }

    return SUCCESS;
}

static int m_handle_cmd(int fd, cJSON *root)
{
    cJSON*  key_value;
    cJSON*  arg;
    int     ret;

    key_value = cJSON_GetObjectItem(root, "cmd");
    if (!key_value || !cJSON_IsString(key_value))
        return ERROR;

    arg = cJSON_GetObjectItem(root, "arg");

    if (arg && cJSON_IsString(arg))
        ret = game_execute_command(fd, key_value->valuestring, arg->valuestring);
    else
        ret = game_execute_command(fd, key_value->valuestring, NULL);

    /* game execute_command must inform to the client if something would happen. */
    return ret;
}

static int m_handle_login_admin(int fd, cJSON *root)
{
    (void)root;

    m_create_json_response(fd, "ko", "Not handled yet", NULL);
    return ERROR;
}

static int m_handle_login_observer(int fd, cJSON *root)
{
    int ret;

    (void)root;
    ret = game_register_observer(fd);
    if (ret == ERROR)
    {
        m_create_json_response(fd, "error", "Failed to register observer", NULL);
        return ERROR;
    }

    m_create_json_response(fd, "ok", "Observer registered", NULL);
    return ret;
}

static int m_handle_login_client(int fd, cJSON *root)
{
    cJSON*  response;
    cJSON*  key_value;
    cJSON*  map_size;
    int     map_x;
    int     map_y;
    int     ret;
    char*   json;

    /**/
    key_value = cJSON_GetObjectItem(root, "team-name");
    if (!key_value || !cJSON_IsString(key_value))
    {
        m_create_json_response(fd, "error", "Invalid team name", NULL);
        return ERROR;
    }

    /**/
    ret = game_register_player(fd, key_value->valuestring);    
    if (ret == ERROR)
    {
        m_create_json_response(fd, "error", "Failed to register player", NULL);
        return ERROR;
    }

    /**/
    response = cJSON_CreateObject();
    if (!response)
        return ERROR;
    
    cJSON_AddStringToObject(response, "type", "welcome");
    cJSON_AddNumberToObject(response, "remaining_clients", game_get_team_remaining_clients(fd));
    
    map_size = cJSON_CreateObject();
    if (!map_size)
    {
        cJSON_Delete(response);
        return ERROR;
    }
    
    game_get_map_size(&map_x, &map_y);

    cJSON_AddNumberToObject(map_size, "x", map_x);
    cJSON_AddNumberToObject(map_size, "y", map_y);
    
    cJSON_AddItemToObject(response, "map_size", map_size);
    
    json = cJSON_Print(response);
    if (!json)
    {
        cJSON_Delete(response);
        return ERROR;
    }
    
    send(fd, json, strlen(json), 0);

    free(json);
    cJSON_Delete(response);

    /* game execute_command must inform to the client if something would happen. */
    return ret;
}

static int m_handle_login(int fd, cJSON *root)
{
    cJSON*  key_value;
    int     ret;
    int     i;

    key_value = cJSON_GetObjectItem(root, "key");
    if (!key_value || !cJSON_IsString(key_value))
        return ERROR;

    if (strcmp(key_value->valuestring, SERVER_KEY) != 0)
    {
        /* Ignore rc as it's already an error
        */
        m_create_json_response(fd, "error", "Invalid key", NULL);
        return ERROR;
    }

    /**/
    key_value = cJSON_GetObjectItem(root, "role");
    if (!key_value || !cJSON_IsString(key_value))
    {
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
        m_create_json_response(fd, "error", "Invalid role", NULL);
        return ERROR;
    }

    ret = m_login_handlers[i](fd, root);
    if (ret == ERROR)
    {
        /* error must be sent by prev func to client. */
        return ERROR;
    }
    return SUCCESS;
}

static int m_handle_client_message(int fd, char *buffer, int bytes)
{
    client_message_type type;
    cJSON *key_value;
    cJSON *root;
    int ret;

    buffer[bytes] = '\0';

    root = cJSON_Parse(buffer);
    if (!root)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to parse JSON!\n");
        log_msg(LOG_LEVEL_ERROR, "Error before: %s\n", cJSON_GetErrorPtr());
        log_msg(LOG_LEVEL_ERROR, "Buffer: %s\n", buffer);
        return ERROR;
    }
    
    key_value = cJSON_GetObjectItem(root, "type");
    if (!key_value || !cJSON_IsString(key_value))
    {
        cJSON_Delete(root);
        m_create_json_response(fd, "error", "Invalid JSON format", NULL);
        return ERROR;
    }

    type = m_get_message_type(key_value->valuestring);
    if (type == type_unknown)
    {
        cJSON_Delete(root);
        m_create_json_response(fd, "error", "Unknown message type", NULL);
        return ERROR;
    }

    ret = m_handlers[type](fd, root);
    if (ret == ERROR)
    {
        cJSON_Delete(root);
        m_create_json_response(fd, "error", "Failed to handle message", NULL);
        return ERROR;
    }

    cJSON_Delete(root);

    return SUCCESS;
}

static int m_handle_client_event(int fd)
{
    char buffer[4096];
    int bytes;
    int ret;
    
    bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
    {
        log_msg(LOG_LEVEL_WARN, "Client fd=%d disconnected or error\n", fd);
        REMOVE_CLIENT(fd);
        return SUCCESS;
    }

    ret = m_handle_client_message(fd, buffer, bytes);
    if (ret == ERROR)
    {
        return ERROR;
    }
    else
    {
    }

    return SUCCESS;
}

static void m_remove_clients()
{
    client_list_t* client;
    client_list_t* next_client;

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
    client_list_t* client;

    if (fd < 0 || fd > m_max_fd)
        return ERROR;

    client = malloc(sizeof(client_list_t));
    client->fd = fd;
    FT_LIST_ADD_LAST(&m_clients, client);

    return SUCCESS;
}

int server_select()
{
    fd_set read_fds;
    int ret;
    int fd;
    struct timeval timeout;

    /* Check for any delegated handshakes */
    ssl_al_lookup_new_clients();

    /* Cpy the read_fds set to avoid modifying the original. */
    memcpy(&read_fds, &m_read_fds, sizeof(m_read_fds));

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    ret = select(m_max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret < 0) /* Error... */
    {
        log_msg(LOG_LEVEL_ERROR, "Select error: (%d) (%d) \n", ret, m_max_fd);
        return ERROR;
    }
    else if (ret == 0) /* No new data to read. */
        return 0;

    for (fd = 0; fd <= m_max_fd; ++fd)
    {
        if (!FD_ISSET(fd, &read_fds))
            continue;

        if (fd == m_sock_server)
        {
            ret = m_handle_new_client(fd);
            if (ret == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR, "Failed to accept new client\n");
                ret = 0;
            }
        }
        else
        {
            ret = m_handle_client_event(fd);
            if (ret == ERROR)
            {
                log_msg(LOG_LEVEL_ERROR, "Failed to handle client event\n");
                /* remove client */
                /* by doing FD_CLR we will just ignore him so he will
                 * be disconnected by his side when tcp timeout occurs
                 */
                REMOVE_CLIENT(fd);
                log_msg(LOG_LEVEL_INFO, "Client fd=%d disconnected\n", fd);
                /* We remove the client but we don't want to close the server at all..
                */
                ret = 0;
            }
        }
    }

    m_remove_clients();
    return ret;
}

int init_server(int port, char* cert, char* key)
{
    m_sock_server = init_ssl_al(cert, key, port, cb_on_accept_success);
    if (m_sock_server == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize SSL\n");
        return ERROR;
    }

    set_server_socket(m_sock_server);
    m_max_fd = m_sock_server;
    FD_ZERO(&m_read_fds);
    log_msg(LOG_LEVEL_BOOT, "Server socket initialized: fd=%d\n", m_sock_server);
    FD_SET(m_sock_server, &m_read_fds);

    return SUCCESS;
}

void cleanup_server()
{
    for (int fd = 0; fd <= m_max_fd; ++fd)
    {
        if (FD_ISSET(fd, &m_read_fds))
        {
            REMOVE_CLIENT(fd);
        }
    }
    cleanup_ssl_al();
    ssl_table_free();
}
