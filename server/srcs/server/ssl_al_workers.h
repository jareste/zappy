#ifndef __SSL_AL_WORKERS_H__
#define __SSL_AL_WORKERS_H__

typedef void (*handshake_done_cb)(int client_fd, SSL *ssl);
void init_handshake_pool(handshake_done_cb done_cb);
int ssl_al_worker_queue(int client_fd, SSL *ssl);
int ssl_al_worker_dequeue_all();

#endif /* __SSL_AL_WORKERS_H__ */