#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <arpa/inet.h>

#include "../include/simd_util.h"

#include "perf_jig.h"

static int perf_test_clmul(const char **args)
{
    const u64 _a = ARG_VALID(args[1]) ? strtoul(args[1], NULL, 0) : 1;
    const u64 _b = ARG_VALID(args[2]) ? strtoul(args[2], NULL, 0) : 1;

    const u64_2 a = {_a, ~_a};
    const u64_2 b = {_b, ~_b};

    const u64_2 c = (u64_2)_mm_clmulepi64_si128((__m128i)a, (__m128i)b, 0);

    printf("%s(0x%lx, 0x%lx) = 0x%016lx%016lx\n", args[0], a[0], b[0], c[1], c[0]);

    u64_2 foo[256];
    randomize_data(foo, sizeof(foo));
    u64_2 accum[8] = {};
    unsigned i;
    const unsigned n = sizeof(foo) / sizeof(foo[0]);

    const u64 pre = TSC_PRECISE();

    for (i = 0; i < n; i++) {
        accum[0] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)a, 0x00);
        accum[1] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)a, 0x01);
        accum[2] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)a, 0x10);
        accum[3] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)a, 0x11);
        accum[4] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)b, 0x00);
        accum[5] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)b, 0x01);
        accum[6] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)b, 0x10);
        accum[7] ^= (u64_2)_mm_clmulepi64_si128((__m128i)foo[i], (__m128i)b, 0x11);
    }

    const u64 post = TSC_PRECISE();
    consume_data(accum, sizeof(accum));

    printf("%s: %u pclmulqdq instructions in %lu clocks.\n", args[0], n * 8, post - pre);
    return 0;
}

static int perf_test_murmur3_notail(const char **args)
{
    char err_buf[1024] = {};
    const unsigned min_dw       = ARG_VALID(args[1]) ? strtoul(args[1], NULL, 0) : 16;
    const unsigned max_dw       = ARG_VALID(args[2]) ? strtoul(args[2], NULL, 0) : 16;
    const unsigned nkeys        = ARG_VALID(args[3]) ? strtoul(args[3], NULL, 0) : (1 << 16);
    const unsigned range        = max_dw - min_dw;
    const unsigned n            = nkeys / 8;
    unsigned i;

    if ((max_dw < min_dw) | (min_dw < 1) | (nkeys & 7)) {
        printf("%s: requires 0 < min_dw <= max_dw and nkeys must be a multiple of 8\n", args[0]);
        return -1;
    }

    const u64 dseg_len = ((n * sizeof(u32_8) * max_dw) + HUGE_2M_MASK) & ~HUGE_2M_MASK;
    // one for key length in DWORDS, one for result
    const u64 out_len = ((n * sizeof(u32_8) * 2) + HUGE_2M_MASK) & ~HUGE_2M_MASK;

    seg_desc_t dseg = {
        .maplen = dseg_len, .flags = SEG_DESC_INITD | SEG_DESC_ANON
    };
    seg_desc_t tseg = {
        .maplen = out_len, .flags = SEG_DESC_INITD | SEG_DESC_ANON
    };

    if (map_segment(NULL, &dseg, err_buf, sizeof(err_buf) - 1)) {
        printf("%s: %s\n", args[0], err_buf);
        return -1;
    }

    if (map_segment(NULL, &tseg, err_buf, sizeof(err_buf) - 1)) {
        unmap_segment(&dseg);
        printf("%s: %s\n", args[0], err_buf);
        return -1;
    }

    randomize_data(dseg.ptr, dseg_len);
    randomize_data(tseg.ptr, out_len);

    u32_8 * const RESTR klen = (u32_8 *)tseg.ptr;
    u32_8 * const RESTR res = klen + n;

    for (i = 0; i < n; i++) {
        klen[i] %= (range + 1);
        klen[i] += min_dw;
    }

    const u64 inc = (sizeof(u32) * max_dw * 8);
    const i64_8 first_inc = IDX_VEC(i64_8) * sizeof(u32) * max_dw;
    const u32_8 seed = IDX_VEC(u32_8) * 42;
    mpv_8 ptrs = {};
    ptrs.vec = first_inc + (u64)dseg.ptr;

    const u64 pre = TSC_PRECISE();

    for (i = 0; i < n; i++) {
        res[i] = murmur3_u32_8_notail(&ptrs, klen[i], seed);
        ptrs.vec += inc;
    }

    const u64 post = TSC_PRECISE();
    const u64 delta = post - pre;

    consume_data(tseg.ptr, out_len);

    unmap_segment(&tseg);
    unmap_segment(&dseg);

    printf("%s: %u keys between %u and %u dwords hashed in %lu clocks...\n", args[0], nkeys, min_dw,
           max_dw, delta);
    printf("\t ~%.2f clk/key\n", (float)delta / (float)nkeys);
    return 0;
}

PERF_FUNC_ENTRY(clmul, "Perform carryless multiply on inputs a and b.", "a", "b");
PERF_FUNC_ENTRY(murmur3_notail,
                "Time 8-way parallel murmur3_32 hash on keys where ((len % 4) == 0).", "min_dw", "max_dw", "nkeys");
