#define _DEFAULT_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "myalloc.h"

/* Each payload must be aligned to a 16-byte boundary. */
#define ALIGNMENT 16
#define MAGIC 0x4D59414C

/* Block header: metadata stored before each payload. */
typedef struct block {
    size_t size;          /* Payload size */
    struct block *next;   /* Next block in address order */
    int free;             /* 1 = free, 0 = allocated */
    int magic;            /* Used to detect invalid pointers */
} block_t;

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
