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

#define PORT 8674
#define ERROR -1
#define SUCCESS 0

static SSL_CTX *m_ctx = NULL;
static int m_sock_server = -1;

void base64_encode(const unsigned char *input, int len, char *output)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines
    BIO_write(b64, input, len);
    BIO_flush(b64);
    BUF_MEM *buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);
    memcpy(output, buffer_ptr->data, buffer_ptr->length);
    output[buffer_ptr->length] = 0;
    BIO_free_all(b64);
}

void websocket_handshake(SSL *ssl, const char *request)
{
    char sec_websocket_key[256] = {0};
    const char *key_header = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_header) return;
    sscanf(key_header, "Sec-WebSocket-Key: %255s", sec_websocket_key);

    // Combine key + magic GUID
    char key_guid[300];
    snprintf(key_guid, sizeof(key_guid), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sec_websocket_key);

    // SHA1 hash
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)key_guid, strlen(key_guid), sha1_hash);

    // Base64 encode
    char accept_key[128];
    base64_encode(sha1_hash, SHA_DIGEST_LENGTH, accept_key);

    // Send response
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    SSL_write(ssl, response, strlen(response));
}

void send_ws_message(SSL *ssl, const char *msg)
{
    size_t len = strlen(msg);
    unsigned char frame[1024] = {0};
    frame[0] = 0x81; // FIN + text frame
    if (len < 126)
    {
        frame[1] = len;
        memcpy(&frame[2], msg, len);
        SSL_write(ssl, frame, len + 2);
    }
}

static int init_server()
{
    m_sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock_server == -1)
    {
        perror("socket");
        return ERROR;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_sock_server, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        fprintf(stderr, "bind failed\n");
        close(m_sock_server);
        return ERROR;
    }
    if (listen(m_sock_server, 1) == -1)
    {
        fprintf(stderr, "listen failed\n");
        close(m_sock_server);
        return ERROR;
    }

    printf("WSS server listening on port %d...\n", PORT);
    return m_sock_server;
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

int init_ssl_al()
{
    int server_sock;
    const SSL_METHOD *method;
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLS_server_method();
    m_ctx = SSL_CTX_new(method);

    if (SSL_CTX_use_certificate_file(m_ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(m_ctx, "key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return ERROR;
    }

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

SSL *ssl_al_accept_client()
{
    if (m_sock_server == -1)
    {
        fprintf(stderr, "Server socket not initialized\n");
        return NULL;
    }

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client = accept(m_sock_server, (struct sockaddr*)&client_addr, &len);
    if (client == -1)
    {
        perror("accept");
        return NULL;
    }

    SSL *ssl = SSL_new(m_ctx);
    SSL_set_fd(ssl, client);

    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client);
        return NULL;
    }

    printf("Client connected: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));


    char buf[4096] = {0};
    SSL_read(ssl, buf, sizeof(buf) - 1);
    printf("Got handshake request:\n%s\n", buf);

    websocket_handshake(ssl, buf);
    printf("Handshake done. Sending welcome message.\n");

    send_ws_message(ssl, "Welcome to WSS WebSocket server!");

    return ssl;
}

int socket_main()
{
    init_ssl_al();
    while (1)
    {
        SSL* ssl = ssl_al_accept_client();
        if (!ssl) {
            fprintf(stderr, "Failed to accept client\n");
            continue;
        }

        unsigned char buffer[1024];
        while (1) {
            int bytes = SSL_read(ssl, buffer, sizeof(buffer));
            if (bytes <= 0) break;
        
            // Decode very basic frame (just text frame with small payload)
            if ((buffer[0] & 0x0F) == 0x1) { // Text frame
                int payload_len = buffer[1] & 0x7F;
                unsigned char *mask = &buffer[2];
                unsigned char *data = &buffer[6];
        
                for (int i = 0; i < payload_len; i++) {
                    data[i] ^= mask[i % 4]; // Unmask
                }
        
                printf("Client says: %.*s\n", payload_len, data);
                send_ws_message(ssl, "Message received!");
            }
        }



        SSL_shutdown(ssl);
        SSL_free(ssl);
    }


    printf("Handshake done. Waiting for messages...\n");

    return 0;
}
