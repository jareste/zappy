#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "../log/log.h"
#include "ssl_al_workers.h"
#include "ssl_table.h"

#define HANDSHAKE_POOL_SIZE  1

typedef struct
{
    int fd;
    SSL *ssl;
} handshake_task_t;

static handshake_task_t task_fds_q[1024];
static handshake_task_t task_fds_dq[1024];
static int task_head_q = 0, task_tail_q = 0;
static int task_head_dq = 0, task_tail_dq = 0;
static pthread_mutex_t task_mtx_q   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t task_mtx_dq   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  task_cond_q  = PTHREAD_COND_INITIALIZER;

static handshake_done_cb on_handshake_done = NULL;

static void *handshake_worker(void *arg)
{
    struct timeval start_time;
    struct timeval end_time;
    SSL *ssl;
    int client_fd;
    
    (void)arg;
    while (1)
    {
        pthread_mutex_lock(&task_mtx_q);
        while (task_head_q == task_tail_q)
        {
            pthread_cond_wait(&task_cond_q, &task_mtx_q);
        }
        client_fd = task_fds_q[task_head_q].fd;
        ssl = task_fds_q[task_head_q].ssl;
        task_head_q = (task_head_q + 1) % 1024;
        pthread_mutex_unlock(&task_mtx_q);

        // log_msg(LOG_LEVEL_DEBUG, "Processing handshake for client fd %d\n", client_fd);
        gettimeofday(&start_time, NULL);
        
        if (SSL_set_fd(ssl, client_fd) == 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        /* Queue it for dequeing from main */
        pthread_mutex_lock(&task_mtx_dq);
        task_fds_dq[task_tail_dq].fd = client_fd;
        task_fds_dq[task_tail_dq].ssl = ssl;
        task_tail_dq = (task_tail_dq + 1) % 1024;
        pthread_mutex_unlock(&task_mtx_dq);          

        /* DEBUG */
        gettimeofday(&end_time, NULL);
        // log_msg(LOG_LEVEL_DEBUG, "Handshake (%d) completed in: %ld microseconds\n", client_fd,
        //        (end_time.tv_sec - start_time.tv_sec) * 1000000L +
        //        (end_time.tv_usec - start_time.tv_usec));
        /* DEBUG_END */

    }
    return NULL;
}

int ssl_al_worker_queue(int client_fd, SSL *ssl)
{
    pthread_mutex_lock(&task_mtx_q);
    task_fds_q[task_tail_q].fd = client_fd;
    task_fds_q[task_tail_q].ssl = ssl;
    task_tail_q = (task_tail_q + 1) % 1024;
    pthread_cond_signal(&task_cond_q);
    pthread_mutex_unlock(&task_mtx_q);
    return 1;
}

int ssl_al_worker_dequeue(int *client_fd, SSL **ssl)
{
    pthread_mutex_lock(&task_mtx_dq);
    if (task_head_dq == task_tail_dq)
    {
        pthread_mutex_unlock(&task_mtx_dq);
        return 0;
    }
    *client_fd = task_fds_dq[task_head_dq].fd;
    *ssl = task_fds_dq[task_head_dq].ssl;
    task_head_dq = (task_head_dq + 1) % 1024;
    pthread_mutex_unlock(&task_mtx_dq);
    return 1;
}

int ssl_al_worker_dequeue_all()
{
    int ret;
    int client_fd;
    SSL *ssl;

    ret = ssl_al_worker_dequeue(&client_fd, &ssl);
    if (ret == 0)
        return 0;

    if (on_handshake_done)
        on_handshake_done(client_fd, ssl);

    while (ret)
    {
        ret = ssl_al_worker_dequeue(&client_fd, &ssl);
        if (ret == 0)
            break;
        if (on_handshake_done)
            on_handshake_done(client_fd, ssl);
    }

    return 1;
}

void init_handshake_pool(handshake_done_cb done_cb)
{
    on_handshake_done = done_cb;
    pthread_t tid;
    int i;

    for (i = 0; i < HANDSHAKE_POOL_SIZE; i++)
    {
        pthread_create(&tid, NULL, handshake_worker, NULL);
        pthread_detach(tid);
    }
}
