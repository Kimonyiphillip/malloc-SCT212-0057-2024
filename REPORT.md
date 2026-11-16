# Memory Allocator Benchmark Report

## 1. Introduction

This experiment evaluates the performance of a custom memory allocator
implemented using the sbrk() system call. The allocator supports first-fit
and best-fit placement policies. Its performance is compared with the
standard C library memory allocation functions.

## 2. Experimental Setup

The benchmark performs 50,000 allocation and deallocation operations.
Allocation sizes range from 1 to 4096 bytes. A fixed random seed of
12345 is used to make the workload reproducible.

The experiment measures execution time, heap size, used memory,
free memory, number of free blocks, largest free block and external
fragmentation where available.

## 3. Results

| Allocator | Execution time (ms) | Heap bytes | Free blocks | Largest free block | Fragmentation |
|-----------|---------------------|------------|-------------|--------------------|---------------|
| First-fit | [actual result] | [actual result] | [actual result] | [actual result] | [actual result] |
| Best-fit | [actual result] | [actual result] | [actual result] | [actual result] | [actual result] |
| libc | [actual result] | N/A | N/A | N/A | N/A |

## 4. Discussion

First-fit searches the free list from the beginning and selects the
first block large enough for the requested allocation. This makes
allocation relatively straightforward and can reduce search time.

Best-fit searches for the smallest free block that can satisfy the
request. It may reduce the unused space left inside selected blocks,
but it can require more searching.

The measured execution times and fragmentation values show how
the two policies behave under the same workload. The standard
library allocator provides a useful performance comparison, although
its internal heap statistics are not directly exposed through this
benchmark.

## 5. Conclusion

The experiment demonstrates that allocation policy affects both
search behaviour and memory fragmentation. The most suitable
policy depends on the workload and the trade-off between allocation
speed and memory utilisation.
