#ifndef SSL_TABLE_H
#define SSL_TABLE_H

#include <openssl/ssl.h>

void ssl_table_init(void);
void ssl_table_free(void);

int  ssl_table_add(int fd, SSL *ssl);
SSL *ssl_table_get(int fd);
void ssl_table_remove(int fd);

#endif
