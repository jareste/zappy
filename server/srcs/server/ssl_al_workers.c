#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "ssl_al_workers.h"
#include "ssl_table.h"

#define HANDSHAKE_POOL_SIZE  4

static int       task_fds[1024];
static int       task_head = 0, task_tail = 0;
static pthread_mutex_t task_mtx   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  task_cond  = PTHREAD_COND_INITIALIZER;

static handshake_done_cb on_handshake_done = NULL;

static SSL_CTX *m_ctx = NULL;

static void *handshake_worker(void *arg)
{
    struct timeval start_time;
    struct timeval end_time;
    SSL *ssl;
    int client_fd;
    
    (void)arg;
    while (1)
    {
        pthread_mutex_lock(&task_mtx);
        while (task_head == task_tail)
        {
            pthread_cond_wait(&task_cond, &task_mtx);
        }
        client_fd = task_fds[task_head];
        task_head = (task_head + 1) % 1024;
        pthread_mutex_unlock(&task_mtx);

        gettimeofday(&start_time, NULL);

        ssl = SSL_new(m_ctx);
        if (!ssl)
        {
            close(client_fd);
            continue;
        }
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        if (on_handshake_done)
            on_handshake_done(client_fd, ssl);
     
        /* DEBUG */
        gettimeofday(&end_time, NULL);
        printf("Handshake completed in: %ld microseconds\n",
               (end_time.tv_sec - start_time.tv_sec) * 1000000L +
               (end_time.tv_usec - start_time.tv_usec));
        /* DEBUG_END */

    }
    return NULL;
}

int ssl_al_worker_queue(int client_fd)
{
    pthread_mutex_lock(&task_mtx);
    task_fds[task_tail] = client_fd;
    task_tail = (task_tail + 1) % 1024;
    pthread_cond_signal(&task_cond);
    pthread_mutex_unlock(&task_mtx);
    return 1;
}

void init_handshake_pool(handshake_done_cb done_cb, SSL_CTX *_m_ctx)
{
    on_handshake_done = done_cb;
    m_ctx = _m_ctx;
    pthread_t tid;
    for (int i = 0; i < HANDSHAKE_POOL_SIZE; i++)
    {
        pthread_create(&tid, NULL, handshake_worker, NULL);
        pthread_detach(tid);
    }
}
