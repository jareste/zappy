#include <stdio.h>
#include "ssl_table.h"
#include "ssl_al.h"
#include <cJSON.h>

#define ERROR -1
#define SUCCESS 0

int socket_main()
{
    int ret;
    unsigned char buffer[4096*4];
    int client;
    struct sockaddr_in client_addr;
    socklen_t len;

    ret = init_ssl_al("certs/cert.pem", "certs/key.pem");
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to initialize SSL\n");
        return ERROR;
    }

    set_server_socket(ret);
    while (1)
    {
        client = accept(ret, (struct sockaddr*)&client_addr, &len);
        if (client == ERROR)
        {
            fprintf(stderr, "Failed to accept client\n");
            continue;
        }

        while (1)
        {
            ret = recv(client, buffer, sizeof(buffer), 0);
            if (ret <= 0)
            {
                fprintf(stderr, "Failed to read from client\n");
                break;
            }
            buffer[ret] = '\0';
            printf("Client says (%d bytes): %s\n", ret, buffer);
    

            cJSON *root2 = cJSON_Parse((char*)buffer);
            if (!root2)
            {
                printf("Failed to parse JSON!\n");
                break;
            }



            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "hello", "Message received!");
            cJSON_AddStringToObject(root, "your_msg", (char*)buffer);

            char *json = cJSON_Print(root);
            printf("JSON: %s\n", json);
            send(client, json, strlen(json), 0);

            cJSON_Delete(root);
            free(json);

            // send(client, buffer, ret, 0);
            // send(client, "Message received!", strlen("Message received!"), 0);
        }

        close(client);
    }

    cleanup_ssl_al();
    return 0;
}