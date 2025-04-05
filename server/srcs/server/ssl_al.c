// wss_server.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "ssl_table.h"
// #include "ssl_al.h"

#define PORT 8674

#define ERROR -1
#define SUCCESS 0

static SSL_CTX *m_ctx = NULL;
static int m_sock_server = -1;

static void base64_encode(const unsigned char *input, int len, char *output)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, len);
    (void)BIO_flush(b64);
    BUF_MEM *buffer_ptr;
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

    (void)flags;
    ssl = ssl_table_get(fd);
    if (!ssl)
        return ERROR;

    while (offset < 2)
    {
        int r = SSL_read(ssl, header + offset, 2 - offset);
        if (r <= 0)
            return ERROR;
        offset += r;
    }

    payload_len = header[1] & 0x7F;
    offset = 2;

    if (payload_len == 126)
    {
        r = SSL_read(ssl, header + offset, 2);
        if (r != 2)
            return ERROR;

        payload_len = (header[offset] << 8) | header[offset + 1];
        offset += 2;
    }
    else if (payload_len == 127)
    {
        r = SSL_read(ssl, header + offset, 8);
        if (r != 8)
            return ERROR;

        payload_len = 0;
        for (int i = 0; i < 8; ++i)
            payload_len = (payload_len << 8) | header[offset + i];
        offset += 8;
    }

    if (payload_len > bufsize)
    {
        fprintf(stderr, "Payload too large for buffer: %zu > %zu\n", payload_len, bufsize);
        return ERROR;
    }

    
    if (SSL_read(ssl, mask, 4) != 4)
        return ERROR;

    dest = buf;
    while (received < payload_len)
    {
        r = SSL_read(ssl, dest + received, payload_len - received);
        if (r <= 0)
            return ERROR;

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

static int init_server()
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
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        fprintf(stderr, "bind failed\n");
        close(sockfd);
        return ERROR;
    }

    if (listen(sockfd, 1) == -1)
    {
        fprintf(stderr, "listen failed\n");
        close(sockfd);
        return ERROR;
    }

    printf("WSS server listening on port %d...\n", PORT);
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

int init_ssl_al(char* cert, char* key)
{
    int server_sock;
    const SSL_METHOD *method;
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLS_server_method();
    m_ctx = SSL_CTX_new(method);

    if (SSL_CTX_use_certificate_file(m_ctx, cert, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(m_ctx, key, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

    /* On failure will simply exit. 
    */
    ssl_table_init();

    server_sock = init_server();
    if (server_sock == ERROR)
    {
        fprintf(stderr, "Failed to initialize server socket\n");
        return ERROR;
    }

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

    ssl = ssl_table_get(fd);
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl_table_remove(fd);
    }
    close(fd);
    return SUCCESS;
}

int ssl_al_accept_client()
{
    char buf[4096] = {0};
    struct sockaddr_in client_addr;
    SSL* ssl;
    socklen_t len;
    int client;
    int ret;

    if (m_sock_server == -1)
    {
        fprintf(stderr, "Server socket not initialized\n");
        return ERROR;
    }

    len = sizeof(client_addr);
    printf("Waiting for client connection...\n");
    client = accept(m_sock_server, (struct sockaddr*)&client_addr, &len);
    if (client == -1)
    {
        perror("accept");
        return ERROR;
    }

    printf("Client connected: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    ssl = SSL_new(m_ctx);
    if (!ssl)
    {
        fprintf(stderr, "Failed to create SSL object\n");
        close(client);
        return ERROR;
    }
    if (SSL_set_fd(ssl, client) == 0)
    {
        fprintf(stderr, "Failed to set file descriptor for SSL\n");
        SSL_free(ssl);
        close(client);
        return ERROR;
    }
    
    ret = ssl_table_add(client, ssl);
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to add SSL to table\n");
        SSL_free(ssl);
        close(client);
        return ERROR;
    }

    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client);
        return ERROR;
    }

    printf("Client connected: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    SSL_read(ssl, buf, sizeof(buf) - 1);
    printf("Got handshake request:\n%s\n", buf);

    websocket_handshake(ssl, buf);
    printf("Handshake done. Sending welcome message.\n");

    ws_send(client, "Welcome to WSS WebSocket server!", strlen("Welcome to WSS WebSocket server!"), 0);

    return client;
}

int socket_main()
{
    int ret;
    unsigned char buffer[4096*4];
    SSL* ssl;
    // int bytes;

    ret = init_ssl_al("certs/cert.pem", "certs/key.pem");
    if (ret == ERROR)
    {
        fprintf(stderr, "Failed to initialize SSL\n");
        return ERROR;
    }

    m_sock_server = ret;
    while (1)
    {
        ssl = ssl_table_get(ssl_al_accept_client());
        if (!ssl)
        {
            fprintf(stderr, "Failed to get SSL from table\n");
            continue;
        }

        while (1)
        {
            ret = ws_read(SSL_get_fd(ssl), buffer, sizeof(buffer), 0);
            if (ret <= 0)
            {
                fprintf(stderr, "Failed to read from client\n");
                break;
            }
            buffer[ret] = '\0';
            printf("Client says (%d bytes): %s\n", ret, buffer);
            ws_send(SSL_get_fd(ssl), buffer, ret, 0);
            ws_send(SSL_get_fd(ssl), "Message received!", strlen("Message received!"), 0);
        }

        close(SSL_get_fd(ssl));
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    cleanup_ssl_al();
    return 0;
}
