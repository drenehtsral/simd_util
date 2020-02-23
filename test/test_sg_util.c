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

static const u64 barfola = 0xDEADBEEFDEADBEEFUL;

static int test_masked_pointer_vectors(void)
{
    const union {
        mpv_8 mpv_in;
        u64 u64[8];
    } u = { .u64 = {
            [7] = 0x0,
            [6] = (u64)(-1L),
            [5] = 0xDEADBEEFUL,
            [4] = (u64)&setup_data,
            [3] = ((~0UL) >> 1),
            [2] = MSB64,
            [1] = 8,
            [0] = (u64)NULL
        }
    };

    const union {
        i64_8 vec;
        u64   u64[8];
    } bad_lanes = { .u64 = {
            [7] = MSB64,
            [6] = MSB64,
            [2] = MSB64,
            [0] = MSB64,
        }
    };

    const mpv_8 mpv_fixed = { .vec = u.mpv_in.vec | bad_lanes.vec };

    const __mmask8 exp_fixup_mask8 = 0b00111010;
    const __mmask8 exp_fixup_mask4 = exp_fixup_mask8 & 0xF;

    mpv_8 mpv8_out = u.mpv_in;
    mpv_4 mpv4_out = u.mpv_in.v4[0];

    const __mmask8 mpv8_mask_out = mpv_8_fixup_mask(&mpv8_out);
    const __mmask8 mpv4_mask_out = mpv_4_fixup_mask(&mpv4_out);

    if (mpv8_mask_out != exp_fixup_mask8) {
        printf(OUT_PREFIX "mpv_8_fixup_mask() did not produce expected results: 0x%x != 0x%x\n",
               mpv8_mask_out, exp_fixup_mask8);
        return -1;
    }

    if (mpv4_mask_out != exp_fixup_mask4) {
        printf(OUT_PREFIX "mpv_4_fixup_mask() did not produce expected results: 0x%x != 0x%x\n",
               mpv4_mask_out, exp_fixup_mask4);
        return -1;
    }

    if (VEC_TO_MASK(mpv8_out.vec != mpv_fixed.vec)) {
        printf(OUT_PREFIX "mpv_8_fixup_mask() did not produce expected normalized pointer vector.\n");
        return -1;
    }

    if (VEC_TO_MASK(mpv4_out.vec != mpv_fixed.v4[0].vec)) {
        printf(OUT_PREFIX "mpv_4_fixup_mask() did not produce expected normalized pointer vector.\n");
        return -1;
    }

    const u64 splat = 0xfedcba9876543210UL;

    mpv_4 foo_ptr_vec = {
        .vec = {(i64)(&splat), 0, (i64)&barfola, 0}
    };
    const u64_4 expected_u64_4_out = {splat, 0, barfola, 0};

    const u64_4 gather_u64_4_out = GATHER_u64_4_FROM_MPV4(foo_ptr_vec);

    if (VEC_TO_MASK(gather_u64_4_out != expected_u64_4_out)) {
        printf(OUT_PREFIX "GATHER_u64_4_FROM_MPV4() did not return expected results.\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{

    if (test_masked_pointer_vectors()) {
        printf(OUT_PREFIX "%s FAIL\n", __FILE__);
        return -1;
    }

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

    consume_data(&accum, sizeof(accum));

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
