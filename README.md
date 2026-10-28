# Custom Memory Allocator (C++)

A memory allocator built from scratch in three stages to understand how `malloc`/`free` work internally.

## Project structure

| File | What it does |
|---|---|
| `pool_allocator.cpp` | Stage 1: fixed-size blocks, free list, O(1) allocate/free |
| `freelist_allocator.cpp` | Stage 2: variable sizes, block header, splitting, first fit |
| `allocator_final.cpp` | Stage 3: O(1) coalescing, separate free list, double-free detection, leak report, benchmark |

## Features (final version)

- Block header (size, free flag, prev pointer); next block is found as `header + size`
- First-fit allocation with block splitting
- Free blocks kept in a separate doubly linked list, so allocation skips used blocks
- Free-list links stored inside the free block's own payload (zero extra memory)
- O(1) coalescing with left and right neighbours on free
- Double-free and invalid-pointer detection
- Leak report when the allocator is destroyed
- Benchmark against `new`/`delete`

## Build and run

```
g++ -std=c++17 -Wall -Wextra allocator_final.cpp -o allocator_final
./allocator_final
```

On Windows use `.\allocator_final`.

## Benchmark

32-byte allocations: N allocations followed by N frees, repeated for 2000 rounds. Default build (no optimisation flags).

| N | custom allocator | new/delete |
|---|---|---|
| 1000 | 98.9 ms | 98.1 ms |
| 2000 | 172.1 ms | 220.1 ms |
| 4000 | 291.7 ms | 387.9 ms |

Time grows linearly with N. The custom allocator is comparable to `new`/`delete` at N=1000 and about 1.3x faster at larger N on this workload.

### Optimisation history

The first version of the final allocator scanned the whole block list on every free (O(n) per free, O(n^2) per round). It took about 149 ms at N=1000 with 100 rounds. Switching to `prev` pointers and O(1) coalescing brought the same test down to about 10.7 ms (roughly 14x faster).

## Limitations

- Single-threaded, no locking
- 8-byte alignment (system `malloc` gives 16)
- First fit can degrade under heavy fragmentation; size-class bins would fix this
- Double-free detection can miss a block that was merged into its left neighbour
- Benchmark uses a single fixed size, which favours this design, so it is not a claim of beating `malloc` in general

## What I learned

Free lists, pointer arithmetic, alignment, block splitting, coalescing, fragmentation, and how to find and fix a performance bottleneck by measuring.