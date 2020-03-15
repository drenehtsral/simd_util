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

#define MAX_PERF_FUNC_ARGS  (16)

static int example_perf_test_function(const char **args)
{
    return 0;
}

typedef struct perf_func_entry {
    typeof(&example_perf_test_function) func;
    const char *desc;
    const char *arg_names[MAX_PERF_FUNC_ARGS];
} perf_func_entry_t;

#define PERF_FUNC_ENTRY(fname, text, rest...)     \
    { .func = &perf_test_##fname, .desc = text, .arg_names = {#fname, ##rest}}
#define TERMINAL_PERF_FUNC_ENTRY    {}
#define ARG_VALID(_a) ({ const char * __a = (_a); (__a && __a[0]); })


static int perf_test_translate_bytes(const char **args)
{
    char err_buf[1024] = {};
    const unsigned wset     = ARG_VALID(args[1]) ? strtoul(args[1], NULL, 0) : 256;
    const unsigned tables   = ARG_VALID(args[2]) ? strtoul(args[2], NULL, 0) : 256;
    const unsigned rounds   = ARG_VALID(args[3]) ? strtoul(args[3], NULL, 0) : 256;

    if (!wset | !tables | !rounds) {
        printf("Working set, tables, and rounds must all be greater than zero.\n");
        return -1;
    }

    seg_desc_t dseg = {
        .maplen = wset * sizeof(u8_64), .flags = SEG_DESC_INITD | SEG_DESC_ANON
    };
    seg_desc_t tseg = {
        .maplen = tables * sizeof(simd_byte_translation_table), .flags = SEG_DESC_INITD | SEG_DESC_ANON
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

    randomize_data(dseg.ptr, wset * sizeof(u8_64));
    randomize_data(tseg.ptr, tables * sizeof(simd_byte_translation_table));

    u8_64 * const RESTR data = (u8_64 *)dseg.ptr;
    simd_byte_translation_table * const RESTR table = (simd_byte_translation_table *)tseg.ptr;

    unsigned i, j, k;
    const u64 pre = TSC_PRECISE();

    for (i = 0; i < rounds; i++) {
        for (j = 0; j < tables; j++) {
            for (k = 0; k < wset; k++) {
                data[k] = translate_bytes_x64(data[k], table + j);
            }
        }
    }

    const u64 post = TSC_PRECISE();

    consume_data(data, wset * sizeof(u8_64));

    const u64 total_iter = (u64)wset * tables * rounds;
    const u64 total_clk = post - pre;
    printf( "%s(%u, %u, %u):\n\t%lu total translate_bytes_x64() calls in %lu cycles.\n"
            "\t(%.1f clocks per call).\n",
            args[0], wset, tables, rounds, total_iter, total_clk,
            (float)total_clk / (float)total_iter);

    unmap_segment(&dseg);
    unmap_segment(&tseg);
    return 0;
}

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

perf_func_entry_t funcs[] = {
    PERF_FUNC_ENTRY(translate_bytes, "Perform byte-translation lookups in 256-entry tables 64 at a time.", "wset", "tables", "rounds"),
    PERF_FUNC_ENTRY(clmul, "Perform carryless multiply on inputs a and b.", "a", "b"),
    PERF_FUNC_ENTRY(murmur3_notail, "Time 8-way parallel murmur3_32 hash on keys where ((len % 4) == 0).", "min_dw", "max_dw", "nkeys"),
    TERMINAL_PERF_FUNC_ENTRY
};

void usage(const char *prog)
{
    printf("Usage:\n\t%s test[:param[:param...]] ...\n\n", prog);
    printf("Where tests are as follows:\n");
    perf_func_entry_t *trav;

    for (trav = funcs; trav && trav->func; trav++) {
        printf("\t%s", trav->arg_names[0]);
        int i;

        for (i = 1; (i < MAX_PERF_FUNC_ARGS) && trav->arg_names[i]; i++) {
            printf(":%s", trav->arg_names[i]);
        }

        printf(" -- %s\n", trav->desc);
    }

    exit(1);
}

static int chop_test_args(char *cmd, char **argv, const unsigned nargv)
{
    char *start, *trav = cmd;
    int i = 0;

    start = trav;

    while(1) {
        const char c = *trav;

        if ((c == ':') | (c == '\0')) {
            argv[i++] = start;
            *trav = '\0';
            start = ++trav;

            if ((c == '\0') | (i >= nargv)) {
                break;
            }
        } else {
            trav++;
        }
    }

    return i;
}

/*
 *      According to reliable sources (Agner Fog) Intel CPUs supporting AVX-512 tend to power down
 * the upper N 128-bit lane groups unless they've been in use recently.  The power-on circutry takes
 * some time during which the CPU will execute 256 and 512 bit operations over a larger number of
 * cycles by batching them though the low 128-bit group until power is stable on the upper lanes.
 *      Thus, before doing any real performance measurement we want to warm up the power circuitry
 * for those upper lanes.  This also will trigger the turbo frequency limitation on CPUs which
 * reduce the maximum turbo frequency when the regulator for the upper lanes is enabled so as to
 * remain in spec overall with respect to power and thermal envelope.
 */
void OPT_NONE warm_up_simd_unit(void)
{
    u32_16 foo_blob[64];
    const unsigned warmup_loops = 50000;
    const unsigned idxmask = (sizeof(foo_blob) / sizeof(foo_blob[0])) - 1;
    unsigned i;

    randomize_data(foo_blob, sizeof(foo_blob));

    u32_16 tmp = foo_blob[idxmask];

    for (i = 0; i < warmup_loops; i++) {
        const unsigned idx = i & idxmask;
        tmp = (u32_16)_mm512_rorv_epi32((__m512i)foo_blob[idx], (__m512i)(tmp & 0x1F));
        foo_blob[idx] = tmp;
    }

    consume_data(foo_blob, sizeof(foo_blob));
}



int main(int argc, char **argv)
{
    char argtmp[1024] = {};
    union {
        char *p[MAX_PERF_FUNC_ARGS];
        const char *cp[0];
    } tav;
    int i;


    if (argc < 2) {
        usage(argv[0]);
    }

    warm_up_simd_unit();

    for (i = 1; i < argc; i++) {
        memset(tav.p, 0, sizeof(tav));
        strncpy(argtmp, argv[i], sizeof(argtmp) - 1);
        chop_test_args(argtmp, tav.p, sizeof(tav) / sizeof(tav.p[0]));
        perf_func_entry_t *trav;

        for (trav = funcs; trav && trav->func; trav++) {
            if (!strcmp(tav.cp[0], trav->arg_names[0])) {
                if (trav->func(tav.cp) < 0) {
                    exit(1);
                } else {
                    break;
                }
            }
        }

        if (trav->func == NULL) {
            printf("Unknown function \"%s\"\n", tav.cp[0]);
            usage(argv[0]);
        }
    }

    return 0;
}
