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
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>


    #define cleanup_ssl_al() NULL
    #define set_server_socket(sock) NULL
    #define ssl_al_lookup_new_clients() NULL

    static inline int init_ssl_al(char* cert, char* key, int port, void* cb)
    {
        int sockfd;
        struct sockaddr_in addr;

        (void)cert;
        (void)key;
        (void)cb;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            return ERROR;
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {
            close(sockfd);
            return ERROR;
        }

        if (listen(sockfd, 3) == -1)
        {
            close(sockfd);
            return ERROR;
        }
        return sockfd;
    }


#endif



#endif /* SSL_AL_H */
