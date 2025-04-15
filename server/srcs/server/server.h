#ifndef SERVER_H
#define SERVER_H

int server_select(int timeout);
void init_server(int port, char* cert, char* key);
void cleanup_server();

#endif /* SERVER_H */
