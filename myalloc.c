
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
/* Obtain more memory from the operating system.
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

/* Allocate memory. */
void *my_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = align_up(size);

    if (size == 0)
        return NULL;

    block_t *block = find_fit(size);

    if (block == NULL) {
        block = grow_heap(size);

        if (block == NULL)
            return NULL;
    }

    split_block(block, size);

    block->free = 0;

    return payload(block);
}

/* Release memory. */
void my_free(void *ptr)
{
    if (ptr == NULL)
        return;

    block_t *block = find_block(ptr);

    if (block == NULL || block->magic != MAGIC) {
        const char message[] = "my_free: invalid pointer\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return;
    }

    if (block->free) {
        const char message[] = "my_free: double free detected\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return;
    }

    block->free = 1;

    coalesce();
}

/* Allocate and zero-initialize memory. */
void *my_calloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0)
        return NULL;

    if (nmemb > SIZE_MAX / size)
        return NULL;

    size_t total = nmemb * size;

    void *ptr = my_malloc(total);

    if (ptr != NULL)
        memset(ptr, 0, total);

    return ptr;
}
/* Resize an allocated memory block. */
void *my_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return my_malloc(size);

    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_t *block = find_block(ptr);

    if (block == NULL || block->magic != MAGIC || block->free)
        return NULL;

    size_t new_size = align_up(size);

    if (new_size == 0)
        return NULL;

    /* Existing block is already large enough. */
    if (new_size <= block->size) {
        split_block(block, new_size);
        coalesce();
        return ptr;
    }

    /* Try to expand into the next physically adjacent free block. */
    while (block->next != NULL &&
           block->next->free &&
           adjacent(block, block->next)) {

        block_t *next = block->next;

        block->size += HDR + next->size;
        block->next = next->next;

        if (block->next != NULL)
            block->next->prev = block;
        else
            tail = block;

        next->magic = 0;

        if (block->size >= new_size) {
            split_block(block, new_size);
            return ptr;
        }
    }

    /* Allocate a new block and copy the old contents. */
    void *new_ptr = my_malloc(size);

    if (new_ptr == NULL)
        return NULL;

    memcpy(new_ptr, ptr, block->size);

    my_free(ptr);

    return new_ptr;
}

/* Collect allocator statistics. */
void heap_stats(heap_stats_t *out)
{
    if (out == NULL)
        return;

    memset(out, 0, sizeof(*out));

    out->heap_bytes = total_heap_bytes;

    block_t *current = head;

    while (current != NULL) {

        out->blocks++;

        if (current->free) {
            out->free_blocks++;
            out->free_bytes += current->size;

            if (current->size > out->largest_free)
                out->largest_free = current->size;

        } else {
            out->used_bytes += current->size;
        }

        current = current->next;
    }
}

/* Display every block using write(). */
void heap_dump(void)
{
    char buffer[160];

    block_t *current = head;

    while (current != NULL) {

        int length = snprintf(
            buffer,
            sizeof(buffer),
            "Address: %p | Size: %zu | %s\n",
            (void *)current,
            current->size,
            current->free ? "FREE" : "USED"
        );

        if (length > 0) {
            size_t output_length = (size_t)length;

            if (output_length >= sizeof(buffer))
                output_length = sizeof(buffer) - 1;

            write(STDOUT_FILENO, buffer, output_length);
        }

        current = current->next;
    }
}

// Change the allocation placement policy.
void set_policy(fit_policy_t p)
{
    if (p == FIT_FIRST || p == FIT_BEST)
        current_policy = p;
}

// Verify the internal consistency of the heap.
int heap_check(void)
{
    block_t *current = head;
    block_t *previous = NULL;

    while (current != NULL) {

        if (current->magic != MAGIC)
            return 0;

        if (((uintptr_t)payload(current) % ALIGNMENT) != 0)
            return 0;

        if (current->size % ALIGNMENT != 0)
            return 0;

        if (current->prev != previous)
            return 0;

        if (previous != NULL) {

            if ((uintptr_t)current <= (uintptr_t)previous)
                return 0;

            if (previous->free &&
                current->free &&
                adjacent(previous, current))
                return 0;
        }

        previous = current;
        current = current->next;
    }

    if (previous != tail)
        return 0;

    if (head != NULL && head->prev != NULL)
        return 0;

    if (tail != NULL && tail->next != NULL)
        return 0;

    return 1;
}
