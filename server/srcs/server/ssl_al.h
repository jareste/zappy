#ifndef SSL_AL_H
#define SSL_AL_H

int socket_main();

#define USE_SSL

#define ERROR -1
#define SUCCESS 0

#ifdef USE_SSL
    /* functions that only make sense with SSL */
    int init_ssl_al(char* cert, char* key);
    int cleanup_ssl_al();

    int ssl_al_accept_client();
    int ws_close(int fd);
    int ws_send(int fd, const void *buf, size_t len, int flags);
    int ws_read(int fd, void *buf, size_t bufsize, int flags);
    #define accept(sockfd, addr, addrlen) ssl_al_accept_client()
    #define send(sockfd, buf, len, flags) ws_send(sockfd, buf, len, flags)
    #define recv(sockfd, buf, len, flags) ws_read(sockfd, buf, len, flags)
    #define close(sockfd) ws_close(sockfd)
#else
    #include <sys/socket.h>
    #define init_ssl_al(cert, key) SUCCESS
    #define cleanup_ssl_al() SUCCESS
#endif



#endif /* SSL_AL_H */
