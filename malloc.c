#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

typedef struct Meta_block
{
    size_t size;
    int free;
    struct Meta_block * next;
    struct Meta_block * prev;
} meta_block;


#define META_SIZE sizeof(meta_block)
#define align4(x) (((((x)-1)>>2)<<2)+4)

meta_block * global_base = NULL;

meta_block * find_free_block(meta_block ** last, size_t size)
{
    meta_block * cur = global_base;
    while(cur)
    {
        *last = cur;
        if(cur->free == 1 && cur->size >= size)
        {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

meta_block * extend_heap(meta_block * last, size_t size)
{
    meta_block * block = sbrk(0);
    if(sbrk(META_SIZE + size) == (void *) -1)
    {
        return NULL;
    }
    block->size = size;
    block->free = 0;
    block->next = NULL;
    block->prev = last;
    if(last)
    {
        last->next = block;
    }
    return block;
}

void split_block(meta_block * block, size_t size)
{
    if(block->size >= (META_SIZE + size + 4))
    {
        meta_block * newblock = (meta_block *)((char*)block + META_SIZE + size);
        newblock->size = block->size - META_SIZE - size;
        newblock->free = 1;
        newblock->prev = block;
        newblock->next = block->next;
        if(newblock->next)
        {
            newblock->next->prev = newblock;
        }
        block->next = newblock;
        block->size = size;
    }
}

void * my_malloc(size_t size)
{
    size = align4(size);

    if(size <= 0)
    {
        return NULL;
    }
    meta_block * block;
    if(!global_base)
    {
        block = extend_heap(NULL, size);
        if(!block)
        {
            return NULL;
        }
        global_base = block;
    }
    else
    {
        meta_block * last = global_base;
        block = find_free_block(&last, size);

        if(block)
        {
            if(block->size - size >= META_SIZE + 4)
            {
                split_block(block, size);
            }
            block->free = 0;
        }
        else
        {
            block = extend_heap(last, size);
            if(!block)
            {
                return NULL;
            }
        }
    }
    return (char *) block + META_SIZE;
}

meta_block * get_block_ptr(void * ptr)
{
    if(!ptr)
    {
        return NULL;
    }
    return (meta_block *)((char *) ptr - META_SIZE);
}

void merge_blocks(meta_block * block)
{
    if(block->next && block->next->free)
    {
        block->size += META_SIZE + block->next->size;
        block->next = block->next->next;
        if(block->next)
        {
            block->next->prev = block;
        }
    }
}

void my_free(void * ptr)
{
    if(!ptr)
    {
        return;
    }
    meta_block * block = get_block_ptr(ptr);
    block->free = 1;

    merge_blocks(block);

    if(block->prev && block->prev->free)
    {
        merge_blocks(block->prev);
        block = block->prev;
    }

    if(block->next==NULL)
    {
        if(block->prev)
        {
            block->prev->next = NULL;
        }
        else
        {
            global_base = NULL;
        }
        brk(block);
    }
}

typedef struct Node {
    int data;           
    struct Node *next; 
} Node;

void push(Node **head_ref, int new_data) {
    Node *new_node = (Node *)my_malloc(sizeof(Node));

    if (!new_node) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    new_node->data = new_data;
    new_node->next = *head_ref;
    *head_ref = new_node;
}

void append(Node **head_ref, int new_data) {
    Node *new_node = (Node *)my_malloc(sizeof(Node));

    if (!new_node) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    new_node->data = new_data;
    new_node->next = NULL;

    if (*head_ref == NULL) {
        *head_ref = new_node;
        return;
    }

    Node *last = *head_ref;
    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
}

void deleteNode(Node **head_ref, int key) {
    Node *temp = *head_ref;
    Node *prev = NULL;

    if (temp != NULL && temp->data == key) {
        *head_ref = temp->next;
        my_free(temp);
        return;
    }

    while (temp != NULL && temp->data != key) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("Key %d not found in the list.\n", key);
        return;
    }

    prev->next = temp->next;
    my_free(temp);
}

void printList(Node *node) {
    while (node != NULL) {
        printf("%d -> ", node->data);
        node = node->next;
    }

    printf("NULL\n");
}

int main() {
    Node *head = NULL;

    append(&head, 10);
    push(&head, 20);
    push(&head, 30);
    append(&head, 40);

    printf("Linked list: ");
    printList(head);

    deleteNode(&head, 20);
    printf("After deleting 20: ");
    printList(head);

    while (head != NULL) {
        Node *temp = head;
        head = head->next;
        my_free(temp);
    }

    return 0;
}