#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

typedef struct {
    unsigned		foo32;
    unsigned    	bar32;
    unsigned long long baz64;
} foo_struct_t;

static foo_struct_t * setup_data(unsigned n)
{
    const unsigned psize = (1 << 21);
    const unsigned pmask = psize - 1;
    /* Round up to the next 2M page */
    const size_t req = ((sizeof(foo_struct_t) * n) + pmask) & ~pmask;
    const int map_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB;

    foo_struct_t * ptr = mmap(NULL, req, PROT_READ | PROT_WRITE, map_flags, -1, 0);

    if (ptr == MAP_FAILED) {
        ptr = mmap(NULL, req, PROT_READ | PROT_WRITE, map_flags & ~MAP_HUGETLB, -1, 0);
    }

    if (ptr == MAP_FAILED) {
        printf(OUT_PREFIX "Cannot allocate 0x%lx bytes via anonymous mmap()!\n", req);
        return NULL;
    }

    memset(ptr, 0, req);

    unsigned i;

    for(i = 0; i < n; i++) {
        foo_struct_t * const RESTR s = ptr + i;
        const unsigned high = i << 8;

        s->foo32 = high | offsetof(foo_struct_t, foo32);
        s->bar32 = high | offsetof(foo_struct_t, bar32);
        s->baz64 = high | offsetof(foo_struct_t, baz64);
    }

    return ptr;
}

#define LOOPS (1U << 13)

int main(int argc, char **argv)
{
    const unsigned arr_dim = 1 << 19;
    const unsigned arr_mask = arr_dim - 1;

    union {
        u32_8 	 u32_8[LOOPS];
        unsigned      u32[0];
    } idx;

    foo_struct_t * const RESTR arr = setup_data(arr_dim);

    if (arr == NULL) {
        return -1;
    }

    if (randomize_data(&idx, sizeof(idx))) {
        printf(OUT_PREFIX "Cannot randomize index values.\n");
        return -1;
    }

    unsigned i;

    for (i = 0; i < LOOPS; i++) {
        idx.u32_8[i] &= arr_mask;
    }

    u32_8 accum = {};

    for (i = 0; i < LOOPS; i++) {
        u64_8 ptrs = {};
        ptrs += (unsigned long long)arr;
        ptrs += ((u64_8)_mm512_cvtepu32_epi64((__m256i)idx.u32_8[i])) * sizeof(foo_struct_t);

        accum += GATHER_u32_8_FROM_STRUCTS(foo_struct_t, foo32, ~0, ptrs);
    }

    debug_print_vec(accum, ~0);

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
