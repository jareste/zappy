#include <stdio.h>
#include <cJSON.h>
#include <sys/select.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include "ssl_table.h"
#include "ssl_al.h"
#include "../game/game.h"

/* defines */
#define SERVER_KEY "SOME_KEY"
#define MAX_LOGIN_ROLES 3

#define REMOVE_CLIENT(fd) \
    do { \
        FD_CLR(fd, &m_read_fds); \
        game_kill_player(fd); \
        close(fd); \
    } while (0)

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

static int m_handle_new_client(int fd)
{
    int new_client;
    int ret;

    new_client = accept(fd, NULL, NULL);
    if (new_client == ERROR)
    {
        perror("accept");
        return ERROR;
    }

    FD_SET(new_client, &m_read_fds);
    if (new_client > m_max_fd)
        m_max_fd = new_client;

    /**/
    ret = m_create_json_response(new_client, "bienvenue", "Whoa! Knock knock, whos there?", NULL);
    if (ret == ERROR)
    {
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
    cJSON_AddNumberToObject(response, "remaining_clients", game_get_client_count());
    
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
        printf("Failed to parse JSON!\n");
        printf("Error before: %s\n", cJSON_GetErrorPtr());
        printf("Buffer: %s\n", buffer);
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
        printf("Client fd=%d disconnected or error\n", fd);
        perror("recv");
        REMOVE_CLIENT(fd);
        return ERROR;
    }

    ret = m_handle_client_message(fd, buffer, bytes);
    if (ret == ERROR)
    {
        return ERROR;
        // printf("Sent '%s' to fd=%d\n", buffer, fd);
    }
    else
    {
        // printf("Handled JSON message from fd=%d\n", fd);
    }
    return SUCCESS;
}

int server_remove_client(int fd)
{
    if (fd < 0 || fd > m_max_fd)
        return ERROR;

    REMOVE_CLIENT(fd);

    return SUCCESS;
}

int server_select(int sel_timeout)
{
    fd_set read_fds;
    struct timeval timeout;
    int ret;
    int fd;

    /* Cpy the read_fds set to avoid modifying the original. */
    memcpy(&read_fds, &m_read_fds, sizeof(m_read_fds));

    timeout.tv_sec = 0;
    timeout.tv_usec = sel_timeout; /* Non-blocking select at all. */

    ret = select(m_max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret < 0) /* Error... */
        return ERROR;
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
                fprintf(stderr, "Failed to accept new client\n");
                return ERROR;
            }
        }
        else
        {
            ret = m_handle_client_event(fd);
            if (ret == ERROR)
            {
                fprintf(stderr, "Failed to handle client event\n");
                fprintf(stderr, "##################################\n");
                /* remove client */
                /* by doing FD_CLR we will just ignore him so he will
                 * be disconnected by his side when tcp timeout occurs
                 */
                REMOVE_CLIENT(fd);
                printf("Client fd=%d disconnected\n", fd);
            }
        }
    }

    return SUCCESS;
}

int init_server(int port, char* cert, char* key)
{
    m_sock_server = init_ssl_al(cert, key, port);
    if (m_sock_server == ERROR)
    {
        fprintf(stderr, "Failed to initialize SSL\n");
        return ERROR;
    }

    set_server_socket(m_sock_server);
    m_max_fd = m_sock_server;
    FD_ZERO(&m_read_fds);
    printf("Server socket initialized: fd=%d\n", m_sock_server);
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
