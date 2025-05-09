#ifndef SSL_AL_H
#define SSL_AL_H

#include <error_codes.h>

#define ERROR -1
#define SUCCESS 0

#ifdef USE_SSL
    #ifndef socklen_t
    typedef unsigned int socklen_t;
    #endif

    #ifndef in_addr
    struct in_addr
    {
        unsigned long s_addr;
    };
    #endif

    #ifndef sockaddr_in
    struct sockaddr_in
    {
        short sin_family;
        unsigned short sin_port;
        struct in_addr sin_addr;
        char sin_zero[8];
    };
    #endif

    typedef int (*callback_success_SSL_accept)(int);

    /* functions that only make sense with SSL */
    int init_ssl_al(char* cert, char* key, int port, callback_success_SSL_accept cb);
    int cleanup_ssl_al();
    void set_server_socket(int sock);
    int ssl_al_lookup_new_clients();

    int ssl_al_accept_client();
    int ws_close(int fd);
    int ws_send(int fd, const void *buf, size_t len, int flags);
    int ws_read(int fd, void *buf, size_t bufsize, int flags);
    #define accept(sockfd, addr, addrlen) ssl_al_accept_client(sockfd, addr, addrlen)
    #define send(sockfd, buf, len, flags) ws_send(sockfd, buf, len, flags)
    #define recv(sockfd, buf, len, flags) ws_read(sockfd, buf, len, flags)
    #define close(sockfd) ws_close(sockfd)
#else
    #include <sys/socket.h>
    #define init_ssl_al(cert, key, port) SUCCESS
    #define cleanup_ssl_al() SUCCESS
    #define set_server_socket(sock) SUCCESS
#endif



#endif /* SSL_AL_H */
