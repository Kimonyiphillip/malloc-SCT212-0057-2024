# ICS 2305 - Custom Memory Allocator
## Student Information

Name: Phillip Kioko Kimonyi
Registration Number:SCT212-0057/2024
Course: BSc Computer Technology
Unit: ICS 2305 - Systems Programming

## Project Description

This project implements a custom dynamic memory allocator in C.
It uses sbrk() to request memory from the operating system and
maintains allocated and free blocks using a doubly linked list.

The allocator supports first-fit and best-fit placement policies,
block splitting, coalescing, calloc, realloc, heap statistics
and heap consistency checking.

## Compilation and Execution

Compile and execute the provided tests:

    make clean
    make

Compile the benchmark:

    gcc -std=c11 -Wall -Wextra -g -O0 -o bench myalloc.c bench.c

Run benchmarks:

    ./bench first
    ./bench best
    ./bench libc

## Task Completion

| Task | Description | Status |
|------|-------------|--------|
| 1 | Block headers and alignment | Completed |
| 2 | Heap growth using sbrk() | Completed |
| 3 | First-fit allocation | Completed |
| 4 | Block splitting | Completed |
| 5 | Freeing and coalescing | Completed |
| 6 | calloc() and realloc() | Completed |
| 7 | Best-fit and heap diagnostics | Completed |
| 8 | Benchmarking and report | Completed after measurements |

## Design Explanation

The allocator stores metadata in a block header before each
allocated payload. Each header records the block size, allocation
status, a validation value, and pointers to the previous and next
blocks.

Payloads and headers are aligned to 16-byte boundaries. When
memory is requested, the allocator searches its free list using
the selected placement policy. If no suitable block exists, it
requests additional memory through sbrk().

When a sufficiently large free block is selected, it can be split into an allocated block and a smaller free block. When memory is released, physically adjacent free blocks 
are merged to reduce external fragmentation.

The allocator also provides calloc() for zero-initialised memory,
realloc() for resizing allocations, heap_stats() for collecting
memory statistics, heap_dump() for displaying block information,
and heap_check() for checking allocator consistency.

## Limitations

The allocator is not thread-safe. It does not return memory to the
operating system after freeing blocks. Its use of sbrk() also means
that external changes to the process break must be considered.

## AI and External Assistance

Development assistance was used for understanding allocator
concepts, debugging and implementation guidance. 
The implementation
was tested against the supplied assignment test harness.

1. Why must the header size be a multiple of 16 as well as the payload sizes? The payload must begin at a 16-byte aligned address to satisfy alignment requirements. If the 
header size were not a multiple of 16, adding it to an aligned header address could produce a misaligned payload. Aligning both ensures that every returned payload address is 
properly aligned. 2. Why can using printf() inside my_malloc() cause infinite recursion? The implementation of printf() may internally request memory using the standard memory 
allocator. If that allocator indirectly calls our custom allocation function, it can trigger another allocation request. This can create recursive calls and potentially cause 
a stack overflow. Using write() avoids relying on formatted output routines that may allocate memory. 3. Which additional field allows finding the previous block in O(1), and 
what is the technique called? A prev pointer stores the address of the previous block. This makes the block list a doubly linked list. It allows the allocator to access the 
previous list node directly without traversing the entire list. 4. Explain internal and external fragmentation. Internal fragmentation is unused space inside an allocated 
block, usually caused by alignment or allocating a block larger than the requested amount. External fragmentation occurs when free memory is divided into separate smaller 
blocks, making it difficult to satisfy a large allocation even when the total free memory is sufficient. 5. Why is memory obtained using sbrk() not automatically returned 
after freeing? The program break represents the end of the process's data segment, and reducing it can interfere with other allocations or memory used by the runtime. A 
possible improvement is to release memory at the end of the heap when the final block is free and physically adjacent to the current program break, using brk() or a negative 
sbrk() increment carefully. 6. How can two neighbouring list blocks fail to be physically adjacent, and what does coalescing do? Two blocks may be neighbours in the linked 
list because they are consecutive entries in allocation order, but other memory allocations may exist between their addresses. Coalescing checks their actual addresses before 
merging them. It combines free blocks only when they are physically adjacent, creating larger reusable free blocks.
