#ifndef SERVER_H
#define SERVER_H

int server_select();
int init_server(int port, char* cert, char* key);
void cleanup_server();

int server_create_response_to_command(int fd, char *cmd, char *arg, char* status);
int server_send_json(int fd, void* resp);
int server_remove_client(int fd);

#endif /* SERVER_H */
