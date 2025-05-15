#ifndef FT_LIST_H
# define FT_LIST_H

typedef struct list_item_s
{
    struct list_item_s* next;
    struct list_item_s* prev;
} list_item_t;

int ft_list_add_last(void** head, void* node);
int ft_list_add_first(void** head, void* node);
void* ft_list_get_next(void** head, void* node);
void* ft_list_get_prev(void** head, void* node);
int ft_list_pop(void** head, void* node);
int ft_list_pop_first(void** head);
int ft_list_pop_last(void** head);
int ft_list_get_size(void** head);
void* ft_list_get_first(void** head);
void* ft_list_get_last(void** head);
int ft_list_find_node(void** head, void* node);

/* Adds node to the last position of the head list. */
#define FT_LIST_ADD_LAST(head, node) ft_list_add_last((void**)(head), (void*)(node))

/* Adds node to the first position of the head list. */
#define FT_LIST_ADD_FIRST(head, node) ft_list_add_first((void**)(head), (void*)(node))

/* get next node of 'node'. */
#define FT_LIST_GET_NEXT(head, node) ft_list_get_next((void**)(head), (void*)(node))

/* gets prev node of 'node' */
#define FT_LIST_GET_PREV(head, node) ft_list_get_prev((void**)(head), (void*)(node))

/* unlinks a node from the head list */
#define FT_LIST_POP(head, node) ft_list_pop((void**)(head), (void*)(node))

/* unlinks first node of the head list */
#define FT_LIST_POP_FIRST(head) ft_list_pop_first((void**)(head))

/* unlinks last node of head list */
#define FT_LIST_POP_LAST(head) ft_list_pop_last((void**)(head))

/* returns size of the list(members of it) */
#define FT_LIST_GET_SIZE(head) ft_list_get_size((void**)(head))

/* returns first node */
#define FT_LIST_GET_FIRST(head) ft_list_get_first((void**)(head))

/* returns last node */
#define FT_LIST_GET_LAST(head) ft_list_get_last((void**)(head))

/* returns OK if given node it's from the list. */
#define FT_LIST_FIND_NODE(head, node) ft_list_find_node((void**)(head), (void*)(node))

#endif /* FT_LIST_H */
