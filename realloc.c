#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#define align4(x) (((((x)-1)>>2)<<2)+4)

typedef struct Meta_block
{
    size_t size;
    int free;
    struct Meta_block * next;
    struct Meta_block * prev;
} meta_block;

#define META_SIZE sizeof(meta_block)

meta_block * global_base = NULL;

meta_block * find_free_block(meta_block ** last, size_t size)
{
    meta_block * cur = global_base;
    while(cur != NULL)
    {
        *last = cur;
        if(cur->free && cur->size >= size)
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
    if(sbrk(META_SIZE+size) == (void *) -1)
    {
        return NULL;
    }
    block->free = 0;
    block->size = size;
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
        meta_block * new_block = (meta_block *)((char *)block + META_SIZE + size);
        new_block->free = 1;
        new_block->size = block->size - META_SIZE - size;
        new_block->next = block->next;
        new_block->prev = block;
        if(new_block->next)
        {
            new_block->next->prev = new_block;
        }
        block->next = new_block;
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
    return (meta_block *) ((char *) ptr - META_SIZE);
}

void merge_block(meta_block * block)
{
    if(block->next && block->next->free)
    {
        block->size += block->next->size + META_SIZE;
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
    merge_block(block);
    if(block->prev && block->prev->free)
    {
        merge_block(block->prev);
        block = block->prev;
    }

    if(block->next == NULL)
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

void * my_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void * ptr = my_malloc(total);
    if(ptr)
    {
        memset(ptr, 0, total);
    }
    return ptr;
}

void * my_realloc(void * ptr, size_t size)
{
    if(!ptr)
    {
        return my_malloc(size);
    }
    if(size == 0)
    {
        my_free(ptr);
        return NULL;
    }
    size = align4(size);
    meta_block * block = get_block_ptr(ptr);

    if(block->size >= size)
    {
        if(block->size - size >= META_SIZE + 4)
        {
            split_block(block, size);
        }
        return ptr;
    }
    else
    {
        if(block->next && block->next->free && block->size + META_SIZE + block->next->size >= size)
        {
            merge_block(block);
            if(block->size - size >= META_SIZE + 4)
            {
                split_block(block, size);
            }
            if(block->size >= size)
            {
                return ptr;
            }
        }
        void * new_ptr = my_malloc(size);
        if(!new_ptr)
        {
            return NULL;
        }
        memcpy(new_ptr, ptr, block->size);
        my_free(ptr);
        return new_ptr;
    }
}

typedef struct {
    int *data;
    int size;
    int capacity;
} DynamicArray;


void init_array(DynamicArray *arr, int initial_capacity) {
    arr->data = (int *)my_malloc(initial_capacity * sizeof(int));
    arr->size = 0;
    arr->capacity = initial_capacity;
}

void resize_array(DynamicArray *arr, int new_capacity) {
    arr->data = (int *)my_realloc(arr->data, new_capacity * sizeof(int));
    arr->capacity = new_capacity;
    if (arr->size > new_capacity) {
        arr->size = new_capacity;
    }
}

void insert_element(DynamicArray *arr, int value) {
    if (arr->size >= arr->capacity) {
        // Double the capacity when full.
        resize_array(arr, arr->capacity * 2);
    }
    arr->data[arr->size++] = value;
}

void remove_element(DynamicArray *arr, int index) {
    if (index < 0 || index >= arr->size) {
        printf("Index %d out of bounds\n", index);
        return;
    }
    for (int i = index; i < arr->size - 1; i++) {
        arr->data[i] = arr->data[i + 1];
    }
    arr->size--;
    
    if (arr->size > 0 && arr->size < arr->capacity / 4) {
        int new_capacity = arr->capacity / 2;
        if (new_capacity < 1) {
            new_capacity = 1;
        }
        resize_array(arr, new_capacity);
    }
}

void print_array(DynamicArray *arr) {
    printf("[");
    for (int i = 0; i < arr->size; i++) {
        printf("%d", arr->data[i]);
        if (i < arr->size - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

void free_array(DynamicArray *arr) {
    my_free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

int main() {
    DynamicArray arr; 
    init_array(&arr, 5); 
    insert_element(&arr, 10); 
    insert_element(&arr, 20); 
    insert_element(&arr, 30); 
    print_array(&arr); 
    remove_element(&arr, 1); 
    print_array(&arr); 
    resize_array(&arr, 10); 
    print_array(&arr); 
    free_array(&arr);
    return 0;
}