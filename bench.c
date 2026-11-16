#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "myalloc.h"

#define OPERATIONS 50000
#define SLOTS 1000

typedef struct {
    int allocate;
    int slot;
    size_t size;
} operation_t;

static operation_t workload[OPERATIONS];

static void *allocate_memory(size_t size, int use_libc)
{
    if (use_libc)
        return malloc(size);

    return my_malloc(size);
}

static void release_memory(void *ptr, int use_libc)
{
    if (use_libc)
        free(ptr);
    else
        my_free(ptr);
}

/* Generate the same workload for every allocator. */
static void generate_workload(void)
{
    srand(12345);

    for (int i = 0; i < OPERATIONS; i++) {

        workload[i].slot = rand() % SLOTS;

        workload[i].allocate = rand() % 100 < 60;

        workload[i].size = 1 + rand() % 4096;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./bench first|best|libc\n");
        return 1;
    }

    int use_libc = strcmp(argv[1], "libc") == 0;

    if (strcmp(argv[1], "first") == 0) {
        set_policy(FIT_FIRST);
    } else if (strcmp(argv[1], "best") == 0) {
        set_policy(FIT_BEST);
    } else if (!use_libc) {
        printf("Invalid allocator policy.\n");
        return 1;
    }

    generate_workload();

    void *pointers[SLOTS] = {0};

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < OPERATIONS; i++) {

        int slot = workload[i].slot;

        if (workload[i].allocate) {

            if (pointers[slot] == NULL) {
                pointers[slot] =
                    allocate_memory(workload[i].size, use_libc);
            }

        } else {

            if (pointers[slot] != NULL) {
                release_memory(pointers[slot], use_libc);
                pointers[slot] = NULL;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) * 1000.0 +
        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n========== BENCHMARK RESULTS ==========\n");

    printf("Allocator: %s\n", argv[1]);
    printf("Operations: %d\n", OPERATIONS);
    printf("Elapsed time: %.3f ms\n", elapsed);

    if (!use_libc) {

        heap_stats_t stats;

        heap_stats(&stats);

        double fragmentation = 0.0;

        if (stats.free_bytes > 0) {
            fragmentation =
                1.0 -
                ((double)stats.largest_free /
                 (double)stats.free_bytes);
        }

        printf("Heap bytes: %zu\n", stats.heap_bytes);
        printf("Used bytes: %zu\n", stats.used_bytes);
        printf("Free bytes: %zu\n", stats.free_bytes);
        printf("Total blocks: %zu\n", stats.blocks);
        printf("Free blocks: %zu\n", stats.free_blocks);
        printf("Largest free block: %zu\n", stats.largest_free);

        printf("External fragmentation: %.4f\n",
               fragmentation);

        printf("Heap check: %s\n",
               heap_check() ? "PASS" : "FAIL");

    } else {

        printf("Heap statistics: Not exposed by standard libc malloc.\n");
        printf("Fragmentation: Not directly available through this benchmark.\n");
    }

    /* Release remaining allocations. */
    for (int i = 0; i < SLOTS; i++) {
        if (pointers[i] != NULL) {
            release_memory(pointers[i], use_libc);
            pointers[i] = NULL;
        }
    }

    printf("=======================================\n");

    return 0;
}
