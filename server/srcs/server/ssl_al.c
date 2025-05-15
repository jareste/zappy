/* SSL opcodes:
    * 0x0: Continuation frame
    * 0x1: Text frame
    * 0x2: Binary frame
    * 0x8: Connection close
    * 0x9: Ping
    * 0xA: Pong
    * 0xB: Reserved for future use
    * 0xC: Reserved for future use
    * 0xD: Reserved for future use
    * 0xE: Reserved for future use
    * 0xF: Reserved for future use
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include "ssl_table.h"
#include "ssl_al_workers.h"
#include "../log/log.h"

static SSL_CTX *m_ctx = NULL;
static int m_sock_server = -1;
typedef int (*callback_success_SSL_accept)(int);

callback_success_SSL_accept m_callback_success_SSL_accept = NULL;

/*  DEBUG */
/*
static struct timeval m_start_time;
static long m_elapsed_us = 0;
static struct timeval m_end_time;
#define START_TIMER \
 do { \
     gettimeofday(&m_start_time, NULL); \
 } while (0)

#define END_TIMER \
 do { \
     gettimeofday(&m_end_time, NULL); \
     m_elapsed_us = (m_end_time.tv_sec - m_start_time.tv_sec) * 1000000L + \
                       (m_end_time.tv_usec - m_start_time.tv_usec); \
     printf("Elapsed time: %ld microseconds\n", m_elapsed_us); \
 } while (0)
*/
/*  DEBUG_END */

static void base64_encode(const unsigned char *input, int len, char *output)
{
    BIO *b64;
    BIO *bio;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, len);

    (void)BIO_flush(b64);
    BIO_get_mem_ptr(b64, &buffer_ptr);
    memcpy(output, buffer_ptr->data, buffer_ptr->length);
    output[buffer_ptr->length] = 0;
    BIO_free_all(b64);
}

static void websocket_handshake(SSL *ssl, const char *request)
{
    char sec_websocket_key[256] = {0};
    char response[512];
    const char *key_header;
    char key_guid[300];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    char accept_key[128];

    key_header = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_header) return;
    sscanf(key_header, "Sec-WebSocket-Key: %255s", sec_websocket_key);

    /* Combine key + magic GUID */
    snprintf(key_guid, sizeof(key_guid), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sec_websocket_key);

    /* SHA1 hash */
    SHA1((unsigned char*)key_guid, strlen(key_guid), sha1_hash);

    /* Base64 encode */
    base64_encode(sha1_hash, SHA_DIGEST_LENGTH, accept_key);

    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    SSL_write(ssl, response, strlen(response));
}

static int ws_send_close(int fd, uint16_t code, const char *reason)
{
    SSL *ssl = ssl_table_get(fd);
    if (!ssl) return -1;

    unsigned char frame[2 + 125];
    size_t len = 0;

    /* Payload: 2-byte close code (network byte order) + optional reason */
    frame[len++] = (code >> 8) & 0xFF;
    frame[len++] = code & 0xFF;

    if (reason)
    {
        size_t reason_len = strlen(reason);
        if (reason_len > 123) reason_len = 123;
        memcpy(frame + len, reason, reason_len);
        len += reason_len;
    }

    unsigned char header[4];
    header[0] = 0x88; /* FIN = 1, opcode = 0x8 (close) */
    header[1] = len;

    /* Send header + payload */
    SSL_write(ssl, header, 2);
    if (len > 0)
        SSL_write(ssl, frame, len);

    return 0;
}

static int m_ws_send_control_frame(SSL *ssl, uint8_t opcode, const void *data, size_t len)
{
    unsigned char frame[2 + 125];
    size_t offset = 0;

    if (len > 125) return -1; /* RFC: control frame payload must be <126 */

    frame[offset++] = 0x80 | opcode; /* FIN = 1, opcode = 0xA (Pong) */
    frame[offset++] = len;
    memcpy(frame + offset, data, len);

    return SSL_write(ssl, frame, offset + len);
}

int ws_read(int fd, void *buf, size_t bufsize, int flags)
{
    unsigned char header[14]; /* Max header size: 2 + 8 + 4 */
    int offset = 0;
    SSL* ssl;
    size_t payload_len;
    int r;
    unsigned char mask[4];
    size_t received = 0;
    unsigned char *dest;
    size_t i;
    int opcode = 0;
    unsigned char ping_payload[125]; /* RFC: ping max size = 125 */

    (void)flags;
    ssl = ssl_table_get(fd);
    if (!ssl)
    {
        log_msg(LOG_LEVEL_ERROR, "SSL not found for fd %d\n", fd);
        return ERROR;
    }

    while (offset < 2)
    {
        r = SSL_read(ssl, header + offset, 2 - offset);
        if (r <= 0)
        {
            log_msg(LOG_LEVEL_ERROR, "SSL_read error[%d]: %d\n", __LINE__, r);
            return ERROR;
        }

        offset += r;
    }

    payload_len = header[1] & 0x7F;
    offset = 2;

    /* Check for ping 
     */
    opcode = header[0] & 0x0F;
    if (opcode == 0x8)
    {
        ws_send_close(fd, 1000, "Bye!");
        return SUCCESS;
    }
    else if (opcode == 0x9)
    {
        /* Ping received, so lets answer with PONG (0xA) */
        r = 0;
        if (payload_len > 0 && payload_len <= sizeof(ping_payload))
        {
            r = SSL_read(ssl, ping_payload, payload_len);
            if (r != (int)payload_len)
            {
                log_msg(LOG_LEVEL_ERROR, "Failed to read ping payload\n");
                /* try sending empty PONG frame */
                m_ws_send_control_frame(ssl, 0xA, NULL, 0);
                return 1;
            }
            m_ws_send_control_frame(ssl, 0xA, ping_payload, payload_len);
        }
        else
            m_ws_send_control_frame(ssl, 0xA, NULL, 0);

        return 1; /* Indicate that we handled the ping */
    }
    else if (opcode != 0x1) /* 2 would mean binary data */
    {
        log_msg(LOG_LEVEL_ERROR, "Unsupported opcode: 0x%x — closing connection\n", opcode);
        ws_send_close(fd, 1002, "Protocol error: unsupported opcode");
        return 0; /* 0 will fall into a disconnect */
    }

    if (payload_len == 126)
    {
        r = SSL_read(ssl, header + offset, 2);
        if (r != 2)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to read extended payload length\n");
            return ERROR;
        }

        payload_len = (header[offset] << 8) | header[offset + 1];
        offset += 2;
    }
    else if (payload_len == 127)
    {
        r = SSL_read(ssl, header + offset, 8);
        if (r != 8)
        {
            log_msg(LOG_LEVEL_ERROR, "Failed to read extended payload length\n");
            return ERROR;
        }

        payload_len = 0;
        for (i = 0; i < 8; ++i)
            payload_len = (payload_len << 8) | header[offset + i];
        offset += 8;
    }

    if (payload_len > bufsize)
    {
        log_msg(LOG_LEVEL_ERROR, "Payload too large for buffer: %zu > %zu\n", payload_len, bufsize);
        return ERROR;
    }
    
    r = SSL_read(ssl, mask, 4);
    if (r != 4)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to read mask: read %d bytes\n", r);
        ws_send_close(fd, 1002, "Malformed frame");
        return 0; /* 0 will fall into a disconnect */
    }

    dest = buf;
    while (received < payload_len)
    {
        r = SSL_read(ssl, dest + received, payload_len - received);
        if (r <= 0)
        {
            if (SSL_get_error(ssl, r) == SSL_ERROR_WANT_READ)
            {
                log_msg(LOG_LEVEL_ERROR, "SSL_read would block\n");
                return ERROR;
            }
            else if (SSL_get_error(ssl, r) == SSL_ERROR_SYSCALL)
            {
                log_msg(LOG_LEVEL_ERROR, "SSL_read syscall error\n");
                return ERROR;
            }
            else
            {
                log_msg(LOG_LEVEL_ERROR, "SSL_read error[%d]: %d\n", __LINE__, r);
                log_msg(LOG_LEVEL_ERROR, "Received %zu bytes\n", received);
                return ERROR;
            }
        }

        received += r;
    }

    for (i = 0; i < payload_len; ++i)
        dest[i] ^= mask[i % 4];

    return (int)payload_len;
}

int ws_send(int fd, const void *buf, size_t len, int flags)
{
    SSL* ssl;
    size_t frame_size;
    unsigned char *frame;
    size_t offset;
    int ret;

    (void)flags;
    ssl = ssl_table_get(fd);
    if (!ssl) return -1;

    frame_size = 2; /* At least 2 bytes for header */
    if (len < 126)
        frame_size += len;
    else if (len <= 0xFFFF)
        frame_size += 2 + len; /* 2 for extended 16-bit length */
    else
        frame_size += 8 + len; /* 8 for extended 64-bit length */

    frame = malloc(frame_size);
    if (!frame) return -1;

    offset = 0;
    frame[offset++] = 0x81; /* FIN = 1, text frame = 0x1 */

    if (len < 126)
    {
        frame[offset++] = len;
    }
    else if (len <= 0xFFFF)
    {
        frame[offset++] = 126;
        frame[offset++] = (len >> 8) & 0xFF;
        frame[offset++] = len & 0xFF;
    }
    else
    {
        frame[offset++] = 127;
        for (int i = 7; i >= 0; --i)
        {
            frame[offset++] = (len >> (8 * i)) & 0xFF;
        }
    }

    memcpy(frame + offset, buf, len);

    ret = SSL_write(ssl, frame, offset + len);
    free(frame);
    return ret;
}

static int _init_server(int port)
{
    struct sockaddr_in addr = {0};
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return ERROR;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "bind failed\n");
        perror("bind");
        close(sockfd);
        return ERROR;
    }

    if (listen(sockfd, 1) == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "listen failed\n");
        close(sockfd);
        return ERROR;
    }

    log_msg(LOG_LEVEL_BOOT, "WSS server listening on port %d...\n", port);
    return sockfd;
}

static int stop_server()
{
    if (m_sock_server != -1)
    {
        close(m_sock_server);
        m_sock_server = -1;
    }
    return SUCCESS;
}

/* Dequeue all delegated handshakes
*/
int ssl_al_lookup_new_clients()
{
    return ssl_al_worker_dequeue_all();
}

void on_handshake_done(int fd, SSL *ssl)
{
    char buf[4096] = {0};
    int ret;

    ret = ssl_table_add(fd, ssl);
    if (ret == ERROR) goto error;

    ret = SSL_read(ssl, buf, sizeof(buf) - 1);
    if (ret <= 0) goto error;

    websocket_handshake(ssl, buf);

    m_callback_success_SSL_accept(fd);
    return ;

error:
    if (fd != -1)
    {
        close(fd);
    }
    return ;
}

int init_ssl_al(char* cert, char* key, int port, callback_success_SSL_accept cb)
{
    int server_sock;
    const SSL_METHOD *method;
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLS_server_method();
    
    m_ctx = SSL_CTX_new(method);

    if (!m_ctx)
    {
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

    if (SSL_CTX_use_certificate_file(m_ctx, cert, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(m_ctx, key, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

    /* On failure will simply exit. 
    */
    ssl_table_init();

    server_sock = _init_server(port);
    if (server_sock == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize server socket\n");
        return ERROR;
    }

    m_callback_success_SSL_accept = cb;
    init_handshake_pool(on_handshake_done);

    return server_sock;
}

int cleanup_ssl_al()
{
    if (m_ctx)
    {
        SSL_CTX_free(m_ctx);
        m_ctx = NULL;
    }
    stop_server();
    return SUCCESS;
}

int ws_close(int fd)
{
    SSL* ssl;

    if (fd == -1) return ERROR;

    ssl = ssl_table_get(fd);
    if (ssl)
    {
        /* review */
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl_table_remove(fd);
    }
    close(fd);
    return SUCCESS;
}

int ssl_al_accept_client()
{
    struct sockaddr_in client_addr;
    SSL* ssl;
    socklen_t len;
    int client;

    ssl = NULL;
    client = -1;
    if (m_sock_server == -1)
        goto error;

    // START_TIMER;
    len = sizeof(client_addr);
    client = accept(m_sock_server, (struct sockaddr*)&client_addr, &len);
    if (client == -1)
        goto error;
    // END_TIMER;

    ssl = SSL_new(m_ctx);

    if (!ssl)
        goto error;
    // START_TIMER;
    ssl_al_worker_queue(client, ssl);
    // END_TIMER;

    return client;

error:
    if (ssl)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
    }

    if (client != -1)
    {
        close(client);
    }
    return ERROR;
}

void set_server_socket(int sock)
{
    m_sock_server = sock;
}
