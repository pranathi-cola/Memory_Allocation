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
        if(cur->free==1 && cur->size >= size)
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
    if(sbrk(META_SIZE+size)==(void *) -1)
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
        meta_block * new_block = (meta_block *)((char *) block + META_SIZE + size);
        new_block->free = 1;
        new_block->size = block->size - META_SIZE - size;
        new_block->next = block->next->next;
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
    if(size<=0)
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
            if(block->size-size >= META_SIZE+4)
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
    return (char *)block + META_SIZE;
}

meta_block * get_block_ptr(void * ptr)
{
    if(!ptr)
    {
        return NULL;
    }
    return (meta_block *)((char *) ptr - META_SIZE);
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

int ** create_matrix(int rows, int cols)
{
    int ** mat = (int **) my_calloc (rows, sizeof(int *));
    for(int i=0; i<rows; ++i)
    {
        mat[i] = (int *) my_calloc (cols, sizeof(int));
    }
    return mat;
}

void fill_matrix(int ** mat, int rows, int cols)
{
    for(int i=0; i<rows; ++i)
    {
        for(int j=0; j<cols; ++j)
        {
            mat[i][j] = i+j;
        }
    }
}

void print_matrix(int ** mat, int rows, int cols)
{
    for(int i=0; i<rows; ++i)
    {
        for(int j=0; j<cols; ++j)
        {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

void free_matrix(int ** mat, int rows, int cols)
{
    for(int i=0; i<rows; ++i)
    {
        my_free(mat[i]);
    }
    my_free(mat);
}

int main() {
    int rows, cols;
    scanf("%d%d", &rows, &cols);

    int ** mat = create_matrix(rows, cols);
    print_matrix(mat, rows, cols);

    fill_matrix(mat, rows, cols);
    print_matrix(mat, rows, cols);

    free_matrix(mat, rows, cols);

    return 0;
}