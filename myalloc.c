
#define _DEFAULT_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "myalloc.h"

#define ALIGNMENT 16
#define MAGIC 0x4D59414C
#define MIN_HEAP_SIZE 65536

typedef struct block {
    size_t size;
    struct block *next;
    struct block *prev;
    int free;
    int magic;
} block_t;

/* Header size is rounded to a multiple of 16. */
#define HDR ((sizeof(block_t) + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1))

static block_t *head = NULL;
static block_t *tail = NULL;

static size_t total_heap_bytes = 0;
static fit_policy_t current_policy = FIT_FIRST;

/* Round a number up to the next multiple of 16. */
static size_t align_up(size_t size)
{
    if (size > SIZE_MAX - (ALIGNMENT - 1))
        return 0;

    return (size + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
}

/* Return the payload address of a block. */
static void *payload(block_t *block)
{
    return (char *)block + HDR;
}

/* Find the block that owns an exact payload pointer. */
static block_t *find_block(void *ptr)
{
    block_t *current = head;

    while (current != NULL) {
        if (payload(current) == ptr)
            return current;

        current = current->next;
    }

    return NULL;
}

/* Check whether two blocks are physically adjacent. */
static int adjacent(block_t *a, block_t *b)
{
    return (char *)a + HDR + a->size == (char *)b;
}

/* Find a suitable free block according to the selected policy. */
static block_t *find_fit(size_t size)
{
    block_t *current = head;
    block_t *best = NULL;

    while (current != NULL) {
        if (current->free && current->size >= size) {

            if (current_policy == FIT_FIRST)
                return current;

            if (best == NULL || current->size < best->size)
                best = current;
        }

        current = current->next;
    }

    return best;
}

/* Split a large free block when enough space remains. */
static void split_block(block_t *block, size_t size)
{
    if (block->size < size + HDR + ALIGNMENT)
        return;

    block_t *new_block =
        (block_t *)((char *)payload(block) + size);

    new_block->size = block->size - size - HDR;
    new_block->free = 1;
    new_block->magic = MAGIC;

    new_block->prev = block;
    new_block->next = block->next;

    if (new_block->next != NULL)
        new_block->next->prev = new_block;
    else
        tail = new_block;

    block->next = new_block;
    block->size = size;
}
* Obtain more memory from the operating system.
 * The minimum growth amount is 64 KB.
 */
static block_t *grow_heap(size_t size)
{
    if (size > SIZE_MAX - HDR)
        return NULL;

    size_t chunk = size + HDR;

    if (chunk < MIN_HEAP_SIZE)
        chunk = MIN_HEAP_SIZE;

    chunk = align_up(chunk);

    if (chunk == 0)
        return NULL;

    void *old_break = sbrk(0);

    if (old_break == (void *)-1)
        return NULL;

    uintptr_t address = (uintptr_t)old_break;

    size_t padding =
        (ALIGNMENT - (address % ALIGNMENT)) % ALIGNMENT;

    if (chunk > SIZE_MAX - padding)
        return NULL;

    size_t total = chunk + padding;

    if (total > (size_t)INTPTR_MAX)
        return NULL;

    void *memory = sbrk((intptr_t)total);

    if (memory == (void *)-1)
        return NULL;

    block_t *block =
        (block_t *)((char *)memory + padding);

    block->size = chunk - HDR;
    block->free = 1;
    block->magic = MAGIC;
    block->next = NULL;
    block->prev = tail;

    if (tail != NULL)
        tail->next = block;
    else
        head = block;

    tail = block;

    total_heap_bytes += total;

    return block;
}

/* Merge physically adjacent free blocks. */
static void coalesce(void)
{
    block_t *current = head;

    while (current != NULL && current->next != NULL) {

        block_t *next = current->next;

        if (current->free &&
            next->free &&
            adjacent(current, next)) {

            current->size += HDR + next->size;

            current->next = next->next;

            if (current->next != NULL)
                current->next->prev = current;
            else
                tail = current;

            next->magic = 0;

        } else {
            current = current->next;
        }
    }
}





/* Round header size up to a multiple of 16. */
#define HDR ((sizeof(block_t) + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1))

/* First and last blocks in our allocator's linked list. */
static block_t *head = NULL;
static block_t *tail = NULL;

/* Total bytes obtained from the operating system. */
static size_t total_heap_bytes = 0;

/* Allocation policy: first-fit is the default. */
static fit_policy_t current_policy = FIT_FIRST;

/* Round a requested size up to a multiple of 16. */
size_t align_up(size_t n)
{
    if (n > SIZE_MAX - (ALIGNMENT - 1)) {
        return 0;
    }

    return (n + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
}

/* Convert a block header address into its payload address. */
void *payload(block_t *block)
{
    return (char *)block + HDR;
}

/* Convert a payload address back into its block header. */
block_t *header(void *ptr)
{
    return (block_t *)((char *)ptr - HDR);
}

/*
 * Temporary API placeholders.
 * We will replace these as we complete the remaining tasks.
 */

void *my_malloc(size_t size)
{
    (void)size;
    return NULL;
}

void my_free(void *ptr)
{
    (void)ptr;
}

void *my_calloc(size_t nmemb, size_t size)
{
    (void)nmemb;
    (void)size;
    return NULL;
}

void *my_realloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return NULL;
}

void heap_stats(heap_stats_t *out)
{
    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        out->heap_bytes = total_heap_bytes;
    }
}

void heap_dump(void)
{
    /* Implemented in Task 7. */
}

void set_policy(fit_policy_t p)
{
    if (p == FIT_FIRST || p == FIT_BEST) {
        current_policy = p;
    }
}

int heap_check(void)
{
    (void)head;
    (void)tail;
    (void)current_policy;
    return 1;
}
