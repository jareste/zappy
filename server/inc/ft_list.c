#include "ft_list.h"
#include "error_codes.h"
#include <stddef.h>

int ft_list_add_last(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;
    list_item_t* last;

    if (!head || !node) return (INVALID_ARGS);

    if (!*head)
    {
        *head = node;
        node->next = node;
        node->prev = node;
        return (OK);
    }

    last = (*head)->prev;
    last->next = node;
    node->prev = last;
    node->next = *head;
    (*head)->prev = node;
    
    return (OK);
}

int ft_list_add_first(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;
    list_item_t* first;

    if (!head || !node)
    {
        return (INVALID_ARGS);
    }

    if (!*head)
    {
        (*head)->next = node;
        (*head)->prev = node;
        return (OK);
    }

    first = *head;
    while (first->prev != *head)
    {
        first = first->prev;
    }

    first->prev = node;
    node->next = first;
    
    return (OK);
}

void* ft_list_get_next(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;

    if (!head || !node)
    {
        return (NULL);
    }

    if (!*head)
    {
        return (NULL);
    }

    if (node->next == *head)
    {
        return (NULL);
    }

    return (node->next);
}

void* ft_list_get_prev(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;

    if (!head || !node)
    {
        return (NULL);
    }

    if (!*head)
    {
        return (NULL);
    }

    if (node->prev == *head)
    {
        return (NULL);
    }

    return (node->prev);
}

int ft_list_pop(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;
    list_item_t* prev;
    list_item_t* next;

    if (!head || !node || !*head)
    {
        return (INVALID_ARGS);
    }

    prev = node->prev;
    next = node->next;

    if (node == node->next && node == node->prev)
    {
        *head = NULL;
    }
    else
    {
        prev->next = next;
        next->prev = prev;

        if (*head == node)
        {
            *head = next;
        }
    }

    node->next = NULL;
    node->prev = NULL;

    return (OK);
}

int ft_list_pop_first(void** _head)
{
    return (ft_list_pop(_head, ft_list_get_first(_head)));
}

int ft_list_pop_last(void** _head)
{
    return (ft_list_pop(_head, ft_list_get_last(_head)));
}

int ft_list_get_size(void** _head)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* current;
    int size;

    if (!head)
    {
        return (0);
    }

    if (!*head)
    {
        return (0);
    }

    current = *head;
    size = 0;
    do
    {
        size++;
        current = current->next;
    } while (current != *head);

    return (size);
}

void* ft_list_get_first(void** _head)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* first;

    if (!head)
    {
        return (NULL);
    }

    if (!*head)
    {
        return (NULL);
    }

    first = *head;
    while (first->prev != *head)
    {
        first = first->prev;
    }

    return (first);
}

void* ft_list_get_last(void** _head)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* last;

    if (!head)
    {
        return (NULL);
    }

    if (!*head)
    {
        return (NULL);
    }

    last = *head;
    while (last->next != *head)
    {
        last = last->next;
    }

    return (last);
}

int ft_list_find_node(void** _head, void* _node)
{
    list_item_t** head = (list_item_t**)_head;
    list_item_t* node = (list_item_t*)_node;
    list_item_t* current;
    int index;

    if (!head || !node)
    {
        return (INVALID_ARGS);
    }

    if (!*head)
    {
        return (INVALID_ARGS);
    }

    current = *head;
    index = 0;
    do
    {
        if (current == node)
        {
            return (index);
        }
        index++;
        current = current->next;
    } while (current != *head);

    return (FAILURE);
}
