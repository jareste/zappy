#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "../log/log.h"
#include "ssl_al_workers.h"
#include "ssl_table.h"

#define HANDSHAKE_POOL_SIZE 1

typedef struct
{
    int  fd;
    SSL *ssl;
} handshake_task_t;

static handshake_task_t task_fds_q[1024];
static handshake_task_t task_fds_dq[1024];
static int task_head_q  = 0, task_tail_q  = 0;
static int task_head_dq = 0, task_tail_dq = 0;

static pthread_mutex_t task_mtx_q  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t task_mtx_dq = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  task_cond_q = PTHREAD_COND_INITIALIZER;

static handshake_done_cb on_handshake_done = NULL;

/* ------------------------------------------------------------------ */
/*  Worker thread                                                      */
/* ------------------------------------------------------------------ */

static void *handshake_worker(void *arg)
{
    struct timeval start_tv;
    struct timeval end_tv;
    long           elapsed_us;
    SSL           *ssl;
    int            client_fd;

    (void)arg;
    log_msg(LOG_LEVEL_DEBUG,
        "[WORKER] Handshake worker thread started\n");

    while (1)
    {
        /* Wait for a task */
        pthread_mutex_lock(&task_mtx_q);
        while (task_head_q == task_tail_q)
            pthread_cond_wait(&task_cond_q, &task_mtx_q);

        client_fd    = task_fds_q[task_head_q].fd;
        ssl          = task_fds_q[task_head_q].ssl;
        task_head_q  = (task_head_q + 1) % 1024;
        pthread_mutex_unlock(&task_mtx_q);

        log_ssl_handshake_start(client_fd);
        gettimeofday(&start_tv, NULL);

        /* Attach fd to the SSL object */
        if (SSL_set_fd(ssl, client_fd) == 0)
        {
            log_ssl_handshake_fail(client_fd, "SSL_set_fd failed");
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        /* Perform TLS handshake */
        if (SSL_accept(ssl) <= 0)
        {
            log_ssl_handshake_fail(client_fd, "SSL_accept failed");
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        gettimeofday(&end_tv, NULL);
        elapsed_us = (end_tv.tv_sec  - start_tv.tv_sec)  * 1000000L
                   + (end_tv.tv_usec - start_tv.tv_usec);

        log_ssl_handshake_done(client_fd, elapsed_us);

        /* Queue for dequeue from main thread */
        pthread_mutex_lock(&task_mtx_dq);
        task_fds_dq[task_tail_dq].fd  = client_fd;
        task_fds_dq[task_tail_dq].ssl = ssl;
        task_tail_dq = (task_tail_dq + 1) % 1024;
        pthread_mutex_unlock(&task_mtx_dq);
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

int ssl_al_worker_queue(int client_fd, SSL *ssl)
{
    pthread_mutex_lock(&task_mtx_q);
    task_fds_q[task_tail_q].fd  = client_fd;
    task_fds_q[task_tail_q].ssl = ssl;
    task_tail_q = (task_tail_q + 1) % 1024;
    pthread_cond_signal(&task_cond_q);
    pthread_mutex_unlock(&task_mtx_q);

    log_msg(LOG_LEVEL_DEBUG,
        "[WORKER] fd=%d queued for TLS handshake\n", client_fd);
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
    *client_fd   = task_fds_dq[task_head_dq].fd;
    *ssl         = task_fds_dq[task_head_dq].ssl;
    task_head_dq = (task_head_dq + 1) % 1024;
    pthread_mutex_unlock(&task_mtx_dq);
    return 1;
}

int ssl_al_worker_dequeue_all(void)
{
    int  ret;
    int  client_fd;
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
    pthread_t tid;
    int       i;

    on_handshake_done = done_cb;
    for (i = 0; i < HANDSHAKE_POOL_SIZE; i++)
    {
        pthread_create(&tid, NULL, handshake_worker, NULL);
        pthread_detach(tid);
        log_msg(LOG_LEVEL_DEBUG,
            "[WORKER] Handshake worker #%d spawned\n", i);
    }
    log_msg(LOG_LEVEL_BOOT,
        "[WORKER] Handshake pool ready (%d thread(s))\n", HANDSHAKE_POOL_SIZE);
}
