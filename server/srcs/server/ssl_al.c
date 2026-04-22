/* SSL / WebSocket abstraction layer
 *
 * WS opcodes:
 *   0x0 Continuation  0x1 Text       0x2 Binary    0x8 Close
 *   0x9 Ping          0xA Pong       0xB-0xF Reserved
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
#include <sys/time.h>
#include <ft_malloc.h>
#include <error_codes.h>
#include "ssl_table.h"
#include "ssl_al_workers.h"
#include "../log/log.h"

static SSL_CTX *m_ctx         = NULL;
static int      m_sock_server = -1;
typedef int (*callback_success_SSL_accept)(int);
callback_success_SSL_accept m_callback_success_SSL_accept = NULL;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static void base64_encode(const unsigned char *input, int len, char *output)
{
    BIO    *b64;
    BIO    *bio;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, len);
    (void)BIO_flush(b64);
    BIO_get_mem_ptr(b64, &buffer_ptr);
    memcpy(output, buffer_ptr->data, buffer_ptr->length);
    output[buffer_ptr->length] = '\0';
    BIO_free_all(b64);
}

static void websocket_handshake(SSL *ssl, const char *request)
{
    char        sec_websocket_key[256] = {0};
    char        response[512];
    const char *key_header;
    char        key_guid[300];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    char        accept_key[128];

    log_msg(LOG_LEVEL_DEBUG, "[WS][HANDSHAKE] Parsing HTTP upgrade request\n");

    key_header = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_header)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[WS][HANDSHAKE] Missing Sec-WebSocket-Key in HTTP request\n");
        return;
    }

    sscanf(key_header, "Sec-WebSocket-Key: %255s", sec_websocket_key);
    log_msg(LOG_LEVEL_DEBUG,
        "[WS][HANDSHAKE] Sec-WebSocket-Key='%s'\n", sec_websocket_key);

    snprintf(key_guid, sizeof(key_guid),
        "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sec_websocket_key);

    SHA1((unsigned char*)key_guid, strlen(key_guid), sha1_hash);
    base64_encode(sha1_hash, SHA_DIGEST_LENGTH, accept_key);

    log_msg(LOG_LEVEL_DEBUG,
        "[WS][HANDSHAKE] Computed Sec-WebSocket-Accept='%s'\n", accept_key);

    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key);

    int write_ret = SSL_write(ssl, response, strlen(response));
    if (write_ret <= 0)
        log_msg(LOG_LEVEL_ERROR,
            "[WS][HANDSHAKE] SSL_write failed: ret=%d\n", write_ret);
    else
        log_msg(LOG_LEVEL_DEBUG,
            "[WS][HANDSHAKE] HTTP 101 sent (%d bytes)\n", write_ret);
}

/* ------------------------------------------------------------------ */
/*  WebSocket control frames                                           */
/* ------------------------------------------------------------------ */

static int ws_send_close(int fd, uint16_t code, const char *reason)
{
    SSL          *ssl = ssl_table_get(fd);
    unsigned char frame[2 + 125];
    size_t        len = 0;
    unsigned char header[2];

    if (!ssl) return -1;

    frame[len++] = (code >> 8) & 0xFF;
    frame[len++] = code & 0xFF;
    if (reason)
    {
        size_t rlen = strlen(reason);
        if (rlen > 123) rlen = 123;
        memcpy(frame + len, reason, rlen);
        len += rlen;
    }

    header[0] = 0x88; /* FIN=1, opcode=CLOSE */
    header[1] = (unsigned char)len;

    SSL_write(ssl, header, 2);
    if (len > 0)
        SSL_write(ssl, frame, len);

    log_ws_frame_out(fd, 0x8, len);
    log_msg(LOG_LEVEL_DEBUG,
        "[WS][CLOSE] fd=%d code=%u reason='%s'\n",
        fd, (unsigned)code, reason ? reason : "");
    return 0;
}

static int m_ws_send_control_frame(SSL *ssl, uint8_t opcode, const void *data, size_t len)
{
    unsigned char frame[2 + 125];
    size_t        offset = 0;

    if (len > 125) return -1;

    frame[offset++] = 0x80 | opcode;
    frame[offset++] = (unsigned char)len;
    if (data && len > 0)
        memcpy(frame + offset, data, len);

    return SSL_write(ssl, frame, offset + len);
}

/* ------------------------------------------------------------------ */
/*  ws_read                                                            */
/* ------------------------------------------------------------------ */

int ws_read(int fd, void *buf, size_t bufsize, int flags)
{
    unsigned char  header[14];
    int            offset = 0;
    SSL           *ssl;
    size_t         payload_len;
    int            r;
    unsigned char  mask[4];
    size_t         received = 0;
    unsigned char *dest;
    size_t         i;
    int            opcode;
    int            masked;
    unsigned char  ping_payload[125];

    (void)flags;

    ssl = ssl_table_get(fd);
    if (!ssl)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[WS][READ] fd=%d — no SSL context found\n", fd);
        return ERROR;
    }

    /* Read the mandatory 2-byte header */
    while (offset < 2)
    {
        r = SSL_read(ssl, header + offset, 2 - offset);
        if (r <= 0)
        {
            int ssl_err = SSL_get_error(ssl, r);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
                return -2; /* would block */
            if (ssl_err == SSL_ERROR_ZERO_RETURN)
            {
                log_msg(LOG_LEVEL_DEBUG,
                    "[WS][READ] fd=%d — peer closed SSL connection\n", fd);
                return 0;
            }
            log_msg(LOG_LEVEL_ERROR,
                "[WS][READ] fd=%d — SSL_read header failed: ret=%d ssl_err=%d\n",
                fd, r, ssl_err);
            return ERROR;
        }
        offset += r;
    }

    payload_len = header[1] & 0x7F;
    masked      = (header[1] & 0x80) != 0;
    opcode      = header[0] & 0x0F;
    offset      = 2;

    log_ws_frame_in(fd, opcode, payload_len, masked);

    /* ---- CLOSE ---- */
    if (opcode == 0x8)
    {
        log_msg(LOG_LEVEL_INFO, "[WS][CLOSE_FRAME] fd=%d\n", fd);
        ws_send_close(fd, 1000, "Bye!");
        return 0; /* treat as disconnect */
    }

    /* ---- PING ---- */
    if (opcode == 0x9)
    {
        if (payload_len >= 126)
        {
            log_msg(LOG_LEVEL_WARN,
                "[WS][PING] fd=%d — invalid ping payload_len=%zu (RFC violation)\n",
                fd, payload_len);
            ws_send_close(fd, 1002, "Protocol error: invalid ping length");
            return 0;
        }

        if (masked)
        {
            r = SSL_read(ssl, mask, 4);
            if (r != 4)
            {
                log_msg(LOG_LEVEL_ERROR,
                    "[WS][PING] fd=%d — failed to read mask\n", fd);
                ws_send_close(fd, 1002, "Malformed ping frame");
                return 0;
            }
        }

        if (payload_len > 0)
        {
            r = SSL_read(ssl, ping_payload, payload_len);
            if (r != (int)payload_len)
            {
                log_msg(LOG_LEVEL_ERROR,
                    "[WS][PING] fd=%d — failed to read payload\n", fd);
                m_ws_send_control_frame(ssl, 0xA, NULL, 0);
                return -69;
            }
            if (masked)
            {
                for (i = 0; i < payload_len; ++i)
                    ping_payload[i] ^= mask[i % 4];
            }
            m_ws_send_control_frame(ssl, 0xA, ping_payload, payload_len);
        }
        else
        {
            m_ws_send_control_frame(ssl, 0xA, NULL, 0);
        }

        log_msg(LOG_LEVEL_TRACE,
            "[WS][PING] fd=%d — pong sent (payload_len=%zu)\n", fd, payload_len);
        log_ws_frame_out(fd, 0xA, payload_len);
        return -69; /* handled ping — caller ignores */
    }

    /* ---- Unsupported opcode ---- */
    if (opcode != 0x1 && opcode != 0x2)
    {
        log_msg(LOG_LEVEL_WARN,
            "[WS][READ] fd=%d — unsupported opcode=0x%x, closing\n", fd, opcode);
        ws_send_close(fd, 1002, "Protocol error: unsupported opcode");
        return 0;
    }

    /* ---- Extended payload length ---- */
    if (payload_len == 126)
    {
        r = SSL_read(ssl, header + offset, 2);
        if (r != 2)
        {
            log_msg(LOG_LEVEL_ERROR,
                "[WS][READ] fd=%d — failed to read 16-bit extended length\n", fd);
            return ERROR;
        }
        payload_len = ((size_t)header[offset] << 8) | header[offset + 1];
        offset += 2;
    }
    else if (payload_len == 127)
    {
        r = SSL_read(ssl, header + offset, 8);
        if (r != 8)
        {
            log_msg(LOG_LEVEL_ERROR,
                "[WS][READ] fd=%d — failed to read 64-bit extended length\n", fd);
            return ERROR;
        }
        payload_len = 0;
        for (i = 0; i < 8; ++i)
            payload_len = (payload_len << 8) | header[offset + i];
        offset += 8;
    }

    if (payload_len > bufsize)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[WS][READ] fd=%d — payload_len=%zu exceeds buffer=%zu\n",
            fd, payload_len, bufsize);
        return ERROR;
    }

    /* ---- Mask ---- */
    r = SSL_read(ssl, mask, 4);
    if (r != 4)
    {
        if (r <= 0)
        {
            int ssl_err = SSL_get_error(ssl, r);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
                return -2;
        }
        log_msg(LOG_LEVEL_ERROR,
            "[WS][READ] fd=%d — failed to read masking key (got %d bytes)\n", fd, r);
        ws_send_close(fd, 1002, "Malformed frame");
        return 0;
    }

    /* ---- Payload ---- */
    dest = buf;
    while (received < payload_len)
    {
        r = SSL_read(ssl, dest + received, payload_len - received);
        if (r <= 0)
        {
            int ssl_err = SSL_get_error(ssl, r);
            if (ssl_err == SSL_ERROR_WANT_READ)
            {
                log_msg(LOG_LEVEL_WARN,
                    "[WS][READ] fd=%d — SSL_read would block after %zu/%zu bytes\n",
                    fd, received, payload_len);
                return ERROR;
            }
            log_msg(LOG_LEVEL_ERROR,
                "[WS][READ] fd=%d — SSL_read payload error: ret=%d ssl_err=%d received=%zu/%zu\n",
                fd, r, ssl_err, received, payload_len);
            return ERROR;
        }
        received += r;
    }

    /* Unmask */
    for (i = 0; i < payload_len; ++i)
        dest[i] ^= mask[i % 4];

    log_msg(LOG_LEVEL_TRACE,
        "[WS][READ] fd=%d — text frame decoded: %zu bytes\n", fd, payload_len);

    return (int)payload_len;
}

/* ------------------------------------------------------------------ */
/*  ws_send                                                            */
/* ------------------------------------------------------------------ */

int ws_send(int fd, const void *buf, size_t len, int flags)
{
    SSL           *ssl;
    size_t         frame_size;
    unsigned char *frame;
    size_t         offset;
    int            ret;

    (void)flags;
    ssl = ssl_table_get(fd);
    if (!ssl)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[WS][SEND] fd=%d — no SSL context, dropping message\n", fd);
        return -1;
    }

    frame_size = 2;
    if (len < 126)         frame_size += len;
    else if (len <= 0xFFFF) frame_size += 2 + len;
    else                   frame_size += 8 + len;

    frame = malloc(frame_size);
    if (!frame) return -1;

    offset = 0;
    frame[offset++] = 0x81; /* FIN=1, opcode=TEXT */

    if (len < 126)
    {
        frame[offset++] = (unsigned char)len;
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
            frame[offset++] = (len >> (8 * i)) & 0xFF;
    }

    memcpy(frame + offset, buf, len);
    ret = SSL_write(ssl, frame, offset + len);
    free(frame);

    log_ws_frame_out(fd, 0x1, len);
    if (ret <= 0)
        log_msg(LOG_LEVEL_ERROR,
            "[WS][SEND] fd=%d — SSL_write failed: ret=%d\n", fd, ret);

    return ret;
}

/* ------------------------------------------------------------------ */
/*  TCP socket setup                                                   */
/* ------------------------------------------------------------------ */

static int _init_server(int port)
{
    struct sockaddr_in addr = {0};
    int sockfd;
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] socket() failed\n");
        perror("socket");
        return ERROR;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] setsockopt(SO_REUSEADDR) failed\n");
        perror("setsockopt");
        close(sockfd);
        return ERROR;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] bind() failed on port %d\n", port);
        perror("bind");
        close(sockfd);
        return ERROR;
    }

    if (listen(sockfd, 10) == -1)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] listen() failed\n");
        perror("listen");
        close(sockfd);
        return ERROR;
    }

    log_msg(LOG_LEVEL_BOOT, "[SSL_AL] WSS server listening on port %d (fd=%d)\n",
        port, sockfd);
    return sockfd;
}

static int stop_server(void)
{
    if (m_sock_server != -1)
    {
        close(m_sock_server);
        m_sock_server = -1;
    }
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Delegated handshake callback (called from worker thread)           */
/* ------------------------------------------------------------------ */

int ssl_al_lookup_new_clients(void)
{
    return ssl_al_worker_dequeue_all();
}

void on_handshake_done(int fd, SSL *ssl)
{
    char buf[4096] = {0};
    int  ret;

    log_ssl_handshake_done(fd, 0); /* timing tracked in worker */

    ret = ssl_table_add(fd, ssl);
    if (ret == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SSL_AL][HANDSHAKE] fd=%d — ssl_table_add failed\n", fd);
        goto error;
    }

    ret = SSL_read(ssl, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SSL_AL][HANDSHAKE] fd=%d — SSL_read for HTTP upgrade failed: "
            "ret=%d ssl_err=%d\n",
            fd, ret, SSL_get_error(ssl, ret));
        goto error;
    }

    log_msg(LOG_LEVEL_DEBUG,
        "[SSL_AL][HANDSHAKE] fd=%d — received %d-byte HTTP upgrade request\n",
        fd, ret);

    websocket_handshake(ssl, buf);
    m_callback_success_SSL_accept(fd);
    return;

error:
    log_msg(LOG_LEVEL_WARN,
        "[SSL_AL][HANDSHAKE] fd=%d — handshake failed, closing\n", fd);
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl_table_remove(fd);
    }
    if (fd != -1)
        close(fd);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

int init_ssl_al(char *cert, char *key, int port, callback_success_SSL_accept cb)
{
    int               server_sock;
    const SSL_METHOD *method;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    method = TLS_server_method();
    m_ctx  = SSL_CTX_new(method);
    if (!m_ctx)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] SSL_CTX_new failed\n");
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

    if (SSL_CTX_use_certificate_file(m_ctx, cert, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(m_ctx,  key,  SSL_FILETYPE_PEM) <= 0)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SSL_AL] Failed to load cert='%s' or key='%s'\n", cert, key);
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

    log_msg(LOG_LEVEL_BOOT,
        "[SSL_AL] Loaded cert='%s' key='%s'\n", cert, key);

    ssl_table_init();

    server_sock = _init_server(port);
    if (server_sock == ERROR)
    {
        log_msg(LOG_LEVEL_ERROR, "[SSL_AL] Failed to create server socket\n");
        return ERROR;
    }

    m_callback_success_SSL_accept = cb;
    init_handshake_pool(on_handshake_done);

    log_msg(LOG_LEVEL_BOOT,
        "[SSL_AL] Initialised — handshake pool running\n");
    return server_sock;
}

int cleanup_ssl_al(void)
{
    if (m_ctx)
    {
        SSL_CTX_free(m_ctx);
        m_ctx = NULL;
    }
    stop_server();
    log_msg(LOG_LEVEL_INFO, "[SSL_AL] Cleaned up\n");
    return SUCCESS;
}

int ws_close(int fd)
{
    SSL *ssl;

    if (fd == -1) return ERROR;

    ssl = ssl_table_get(fd);
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl_table_remove(fd);
    }
    close(fd);
    log_msg(LOG_LEVEL_DEBUG, "[SSL_AL][CLOSE] fd=%d closed\n", fd);
    return SUCCESS;
}

int ssl_al_accept_client(void)
{
    struct sockaddr_in client_addr;
    SSL               *ssl    = NULL;
    socklen_t          len    = sizeof(client_addr);
    int                client = -1;
    char               ip_str[INET_ADDRSTRLEN] = "?";

    if (m_sock_server == -1)
        goto error;

    client = accept(m_sock_server, (struct sockaddr*)&client_addr, &len);
    if (client == -1)
        goto error;

    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    log_net_connect(client, ip_str);
    log_ssl_handshake_start(client);

    ssl = SSL_new(m_ctx);
    if (!ssl)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[SSL_AL][ACCEPT] fd=%d — SSL_new failed\n", client);
        goto error;
    }

    ssl_al_worker_queue(client, ssl);
    return client;

error:
    if (ssl)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
    }
    if (client != -1)
        close(client);
    return ERROR;
}

void set_server_socket(int sock)
{
    m_sock_server = sock;
}
