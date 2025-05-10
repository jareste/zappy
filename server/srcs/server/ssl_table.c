#include "ssl_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <error_codes.h>
#include <ft_malloc.h>
#include "../log/log.h"

#define INITIAL_BUCKET_COUNT 128

typedef struct ssl_entry
{
    int fd;
    SSL *ssl;
    struct ssl_entry *next;
} ssl_entry;

typedef struct
{
    ssl_entry **buckets;
    size_t bucket_count;
} ssl_table_t;

static bool ssl_table_initialized = false;
static ssl_table_t table;

static size_t hash_fd(int fd)
{
    return (size_t)fd % table.bucket_count;
}

void ssl_table_init(void)
{
    table.bucket_count = INITIAL_BUCKET_COUNT;
    table.buckets = calloc(table.bucket_count, sizeof(ssl_entry *));
    if (!table.buckets)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    ssl_table_initialized = true;
}

void ssl_table_free(void)
{
    if (!ssl_table_initialized)
    {
        log_msg(LOG_LEVEL_ERROR, "SSL table not initialized\n");
        return;
    }

    for (size_t i = 0; i < table.bucket_count; ++i)
    {
        ssl_entry *entry = table.buckets[i];
        while (entry)
        {
            ssl_entry *tmp = entry;
            entry = entry->next;
            free(tmp);
        }
    }
    free(table.buckets);
    table.buckets = NULL;
    table.bucket_count = 0;
}

int ssl_table_add(int fd, SSL *ssl)
{
    if (!ssl_table_initialized)
    {
        log_msg(LOG_LEVEL_ERROR, "SSL table not initialized\n");
        return ERROR;
    }
    size_t index = hash_fd(fd);
    ssl_entry *head = table.buckets[index];

    for (ssl_entry *e = head; e != NULL; e = e->next)
    {
        if (e->fd == fd)
        {
            e->ssl = ssl;
            return SUCCESS;
        }
    }

    ssl_entry *new_entry = malloc(sizeof(ssl_entry));
    if (!new_entry) return ERROR;

    new_entry->fd = fd;
    new_entry->ssl = ssl;
    new_entry->next = head;
    table.buckets[index] = new_entry;
    return SUCCESS;
}

SSL *ssl_table_get(int fd)
{
    if (!ssl_table_initialized)
    {
        log_msg(LOG_LEVEL_ERROR, "SSL table not initialized\n");
        return NULL;
    }
    size_t index = hash_fd(fd);
    for (ssl_entry *e = table.buckets[index]; e != NULL; e = e->next)
    {
        if (e->fd == fd)
        {
            return e->ssl;
        }
    }
    return NULL;
}

void ssl_table_remove(int fd)
{
    if (!ssl_table_initialized)
    {
        log_msg(LOG_LEVEL_ERROR, "SSL table not initialized\n");
        return;
    }
    size_t index = hash_fd(fd);
    ssl_entry *prev = NULL;
    ssl_entry *curr = table.buckets[index];

    while (curr)
    {
        if (curr->fd == fd)
        {
            if (prev) prev->next = curr->next;
            else table.buckets[index] = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}
