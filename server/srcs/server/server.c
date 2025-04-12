#include <stdio.h>
#include <cJSON.h>
#include <sys/select.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include "ssl_table.h"
#include "ssl_al.h"

static int m_sock_server = -1;
static int m_max_fd = -1;
static fd_set m_read_fds;

static int m_handle_new_client(int fd)
{
    int new_client = accept(fd, NULL, NULL);
    if (new_client == ERROR)
    {
        perror("accept");
        return ERROR;
    }

    FD_SET(new_client, &m_read_fds);
    if (new_client > m_max_fd)
        m_max_fd = new_client;

    printf("New client accepted: fd=%d\n", new_client);
    return SUCCESS;
}

static int m_handle_client_message(int fd, char *buffer, int bytes)
{
    buffer[bytes] = '\0';
    // printf("Received from fd=%d: %s\n", fd, buffer);

    cJSON *root = cJSON_Parse(buffer);
    if (!root)
    {
        printf("Failed to parse JSON!\n");
        return ERROR;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "hello", "Message received!");
    cJSON_AddStringToObject(response, "your_msg", buffer);

    char *json = cJSON_Print(response);
    send(fd, json, strlen(json), 0);

    cJSON_Delete(response);
    free(json);

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
        FD_CLR(fd, &m_read_fds);
        close(fd);
        return ERROR;
    }

    ret = m_handle_client_message(fd, buffer, bytes);
    if (ret == ERROR)
    {
        printf("Received '%s' from fd=%d\n", buffer, fd);
        send(fd, buffer, bytes, 0);
        send(fd, "Message Received!!", strlen("Message Received!!"), 0);
        // printf("Sent '%s' to fd=%d\n", buffer, fd);
    }
    else
    {
        // printf("Handled JSON message from fd=%d\n", fd);
    }
    return SUCCESS;
}

int server_select()
{
    fd_set read_fds;
    struct timeval timeout;
    int ret;

    /* Cpy the read_fds set to avoid modifying the original. */
    memcpy(&read_fds, &m_read_fds, sizeof(m_read_fds));

    timeout.tv_sec = 0;
    timeout.tv_usec = 0; /* Non-blocking select at all. */

    ret = select(m_max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret < 0) /* Error... */
        return ERROR;
    else if (ret == 0) /* No new data to read. */
        return 0;

    for (int fd = 0; fd <= m_max_fd; ++fd)
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
            }
        }
    }

    return SUCCESS;
}

void init_server(int port, char* cert, char* key)
{
    m_sock_server = init_ssl_al(cert, key, port);
    if (m_sock_server == ERROR)
    {
        fprintf(stderr, "Failed to initialize SSL\n");
        exit(EXIT_FAILURE);
    }

    set_server_socket(m_sock_server);
    m_max_fd = m_sock_server;
    FD_ZERO(&m_read_fds);
    printf("Server socket initialized: fd=%d\n", m_sock_server);
    FD_SET(m_sock_server, &m_read_fds);
}

void cleanup_server()
{
    for (int fd = 0; fd <= m_max_fd; ++fd)
    {
        if (FD_ISSET(fd, &m_read_fds))
        {
            close(fd);
            FD_CLR(fd, &m_read_fds);
        }
    }
    cleanup_ssl_al();
}

