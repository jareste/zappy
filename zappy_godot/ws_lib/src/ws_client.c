#include "ws_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

#define WS_DEFAULT_IP   "127.0.0.1"
#define WS_DEFAULT_PORT 8674

// Get server IP from environment variable or use default
static const char* get_server_ip() {
    const char* env_ip = getenv("ZAPPY_SERVER_IP");
    return env_ip ? env_ip : WS_DEFAULT_IP;
}

// Get server port from environment variable or use default  
static int get_server_port() {
    const char* env_port = getenv("ZAPPY_SERVER_PORT");
    return env_port ? atoi(env_port) : WS_DEFAULT_PORT;
}

#define MSG_QUEUE_SIZE 64
#define MSG_MAX_LEN    2048

typedef struct
{
    char *msgs[MSG_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
} msg_queue_t;

static int ws_fd = -1;
static SSL_CTX *ws_ctx = NULL;
static SSL *ws_ssl = NULL;

static pthread_t recv_thread;
static int running = 0;

static msg_queue_t msg_queue;

static void msg_queue_init(msg_queue_t *q)
{
    memset(q->msgs, 0, sizeof(q->msgs));
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
}

static void msg_queue_push(msg_queue_t *q, const char *msg)
{
    pthread_mutex_lock(&q->mutex);
    if (q->count == MSG_QUEUE_SIZE)
    {
        free(q->msgs[q->head]);
        q->head = (q->head + 1) % MSG_QUEUE_SIZE;
        q->count--;
    }
    q->msgs[q->tail] = strdup(msg);
    q->tail = (q->tail + 1) % MSG_QUEUE_SIZE;
    q->count++;
    pthread_mutex_unlock(&q->mutex);
}

static int msg_queue_pop(msg_queue_t *q, char *buf, int max_len)
{
    int len = 0;
    char *msg;

    pthread_mutex_lock(&q->mutex);
    if (q->count == 0)
    {
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    msg = q->msgs[q->head];
    q->msgs[q->head] = NULL;
    q->head = (q->head + 1) % MSG_QUEUE_SIZE;
    q->count--;
    pthread_mutex_unlock(&q->mutex);

    if (!msg)
    {
        return 0;
    }
    len = (int)strlen(msg);
    if (len >= max_len)
    {
        len = max_len - 1;
    }
    memcpy(buf, msg, len);
    buf[len] = '\0';
    free(msg);
    return len;
}

static int tcp_connect_blocking(const char *ip, int port)
{
    struct sockaddr_in addr;
    int fd;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("[C] socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        perror("[C] inet_pton");
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[C] connect");
        close(fd);
        return -1;
    }

    return fd;
}

static int base64_encode(const unsigned char *in, int in_len, char *out, int out_size)
{
    int out_len;
    int needed = 4 * ((in_len + 2) / 3) + 1; /* +1 '\0' */
    if (out_size < needed) return -1;

    out_len = EVP_EncodeBlock((unsigned char *)out, in, in_len);
    if (out_len < 0) return -1;
    out[out_len] = '\0';
    return out_len;
}

static int websocket_handshake_client(SSL *ssl)
{
    unsigned char key_raw[16];
    char key_b64[64];
    char req[512];
    char resp[1024];
    int n;
    int req_len;

    if (RAND_bytes(key_raw, sizeof(key_raw)) != 1)
    {
        fprintf(stderr, "[C] RAND_bytes failed\n");
        return -1;
    }

    if (base64_encode(key_raw, sizeof(key_raw), key_b64, sizeof(key_b64)) < 0)
    {
        fprintf(stderr, "[C] base64_encode failed\n");
        return -1;
    }

    req_len = snprintf(
        req, sizeof(req),
        "GET / HTTP/1.1\r\n"
        "Host: " WS_SERVER_IP ":%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        WS_SERVER_PORT, key_b64
    );

    if (req_len <= 0 || req_len >= (int)sizeof(req))
    {
        fprintf(stderr, "[C] handshake request too big\n");
        return -1;
    }

    if (SSL_write(ssl, req, req_len) <= 0)
    {
        fprintf(stderr, "[C] SSL_write handshake failed\n");
        return -1;
    }

    n = SSL_read(ssl, resp, sizeof(resp) - 1);
    if (n <= 0)
    {
        fprintf(stderr, "[C] SSL_read handshake response failed\n");
        return -1;
    }
    resp[n] = '\0';

    if (strstr(resp, " 101 ") == NULL && strstr(resp, "101 Switching") == NULL)
    {
        fprintf(stderr, "[C] WebSocket handshake failed, response:\n%s\n", resp);
        return -1;
    }

    printf("[C] WebSocket handshake OK\n");
    return 0;
}

static int ws_send_text_frame(const char *msg)
{
    size_t len;
    size_t header_extra = 0;
    size_t frame_len;
    unsigned char *frame;
    size_t off = 0;
    size_t i;
    unsigned char mask[4];
    int ret;
    int err;
    
    if (!ws_ssl) return -1;
    len = strlen(msg);

    if (len < 126) header_extra = 0;
    else if (len <= 0xFFFF) header_extra = 2;
    else header_extra = 8;

    frame_len = 2 + header_extra + 4 + len;
    frame = malloc(frame_len);
    if (!frame) return -1;

    frame[off++] = 0x81; /* FIN = 1, opcode = 0x1 (text) */

    if (len < 126)
    {
        frame[off++] = 0x80 | (unsigned char)len;
    }
    else if (len <= 0xFFFF)
    {
        frame[off++] = 0x80 | 126;
        frame[off++] = (len >> 8) & 0xFF;
        frame[off++] = len & 0xFF;
    }
    else
    {
        frame[off++] = 0x80 | 127;
        for (i = 7; i >= 0; --i)
        {
            frame[off++] = (len >> (8 * i)) & 0xFF;
        }
    }

    if (RAND_bytes(mask, sizeof(mask)) != 1)
    {
        free(frame);
        return -1;
    }

    for (i = 0; i < 4; ++i)
        frame[off++] = mask[i];

    for (i = 0; i < len; ++i)
    {
        frame[off++] = ((unsigned char)msg[i]) ^ mask[i % 4];
    }

    ret = SSL_write(ws_ssl, frame, (int)frame_len);
    free(frame);
    if (ret <= 0)
    {
        err = SSL_get_error(ws_ssl, ret);
        fprintf(stderr, "[C] SSL_write frame error: %d\n", err);
        return -1;
    }
    return ret;
}

static int ws_recv_frame_blocking(char *out, int out_size)
{
    unsigned char header[2];
    int n;
    int err;
    int fin;
    int masked;
    unsigned char b0;
    unsigned char b1;
    unsigned char opcode;
    uint64_t payload_len;
    uint64_t remaining;
    unsigned char ext[8];
    unsigned char mask[4];
    char tmp[512];
    int chunk;
    size_t received;
    uint64_t i;

    n = SSL_read(ws_ssl, header, 2);
    if (n <= 0) {
        err = SSL_get_error(ws_ssl, n);
        fprintf(stderr, "[C] SSL_read header error: %d\n", err);
        return -1;
    }

    b0 = header[0];
    b1 = header[1];

    fin = (b0 & 0x80) != 0;
    opcode = b0 & 0x0F;
    masked = (b1 & 0x80) != 0;
    payload_len = (b1 & 0x7F);

    if (!fin)
    {
        fprintf(stderr, "[C] fragmented frames not supported\n");
        return -1;
    }

    if (payload_len == 126)
    {
        n = SSL_read(ws_ssl, ext, 2);
        if (n != 2) return -1;
        payload_len = ((uint64_t)ext[0] << 8) | ext[1];
    }
    else if (payload_len == 127)
    {
        n = SSL_read(ws_ssl, ext, 8);
        if (n != 8) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; ++i)
        {
            payload_len = (payload_len << 8) | ext[i];
        }
    }

    if (masked)
    {
        n = SSL_read(ws_ssl, mask, 4);
        if (n != 4) return -1;
    }

    if (payload_len + 1 > (uint64_t)out_size)
    {
        fprintf(stderr, "[C] frame too big: %llu\n", (unsigned long long)payload_len);
        remaining = payload_len;
        while (remaining > 0)
        {
            chunk = (remaining > sizeof(tmp)) ? sizeof(tmp) : (int)remaining;
            n = SSL_read(ws_ssl, tmp, chunk);
            if (n <= 0) break;
            remaining -= n;
        }
        return -1;
    }

    received = 0;
    while (received < payload_len)
    {
        n = SSL_read(ws_ssl, out + received, (int)(payload_len - received));
        if (n <= 0)
        {
            int err = SSL_get_error(ws_ssl, n);
            fprintf(stderr, "[C] SSL_read payload error: %d\n", err);
            return -1;
        }
        received += n;
    }

    if (masked)
    {
        for (i = 0; i < payload_len; ++i)
        {
            out[i] ^= mask[i % 4];
        }
    }

    if (opcode == 0x8)
    {
        fprintf(stderr, "[C] received CLOSE frame\n");
        return -1;
    }
    else if (opcode == 0x9)
    {
        unsigned char pong_hdr[2] = {0x8A, (unsigned char)payload_len};
        SSL_write(ws_ssl, pong_hdr, 2);
        if (payload_len > 0)
        {
            SSL_write(ws_ssl, out, (int)payload_len);
        }
        return 0;
    }
    else if (opcode != 0x1)
    {
        fprintf(stderr, "[C] unsupported opcode: 0x%x\n", opcode);
        return 0;
    }

    out[payload_len] = '\0';
    return (int)payload_len;
}

static void *recv_thread_func(void *arg)
{
    int n;
    char buf[MSG_MAX_LEN];
    
    (void)arg;
    while (running)
    {
        n = ws_recv_frame_blocking(buf, sizeof(buf));
        if (!running) break;
        if (n > 0)
        {
            msg_queue_push(&msg_queue, buf);
        }
        else if (n < 0)
        {
            fprintf(stderr, "[C] recv_thread: error or close, stopping\n");
            break;
        }
        else
        {
            continue;
        }
    }

    running = 0;
    return NULL;
}


int ws_init(void)
{
    if (ws_ssl)
    {
        return 0;
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ws_ctx = SSL_CTX_new(TLS_client_method());
    if (!ws_ctx)
    {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    ws_fd = tcp_connect_blocking(WS_SERVER_IP, WS_SERVER_PORT);
    if (ws_fd < 0)
    {
        return -1;
    }

    ws_ssl = SSL_new(ws_ctx);
    if (!ws_ssl)
    {
        ERR_print_errors_fp(stderr);
        close(ws_fd);
        ws_fd = -1;
        return -1;
    }

    SSL_set_fd(ws_ssl, ws_fd);

    if (SSL_connect(ws_ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ws_ssl);
        ws_ssl = NULL;
        close(ws_fd);
        ws_fd = -1;
        return -1;
    }

    printf("[C] TLS handshake OK\n");

    if (websocket_handshake_client(ws_ssl) < 0)
    {
        fprintf(stderr, "[C] websocket_handshake_client failed\n");
        ws_close();
        return -1;
    }

    msg_queue_init(&msg_queue);
    running = 1;
    if (pthread_create(&recv_thread, NULL, recv_thread_func, NULL) != 0)
    {
        perror("[C] pthread_create");
        running = 0;
        ws_close();
        return -1;
    }

    printf("[C] WSS + WebSocket client initialized\n");
    return 0;
}

int ws_send(const char *msg)
{
    if (!ws_ssl)
    {
        fprintf(stderr, "[C] ws_send: not initialized\n");
        return -1;
    }
    return ws_send_text_frame(msg);
}

int ws_recv(char *buf, int max_len)
{
    if (!ws_ssl || !running)
    {
        return -1;
    }
    return msg_queue_pop(&msg_queue, buf, max_len);
}

void ws_close(void)
{
    if (running)
    {
        running = 0;
        pthread_join(recv_thread, NULL);
    }

    if (ws_ssl)
    {
        SSL_shutdown(ws_ssl);
        SSL_free(ws_ssl);
        ws_ssl = NULL;
    }
    if (ws_fd != -1)
    {
        close(ws_fd);
        ws_fd = -1;
    }
    if (ws_ctx)
    {
        SSL_CTX_free(ws_ctx);
        ws_ctx = NULL;
    }
    printf("[C] WSS client closed\n");
}
