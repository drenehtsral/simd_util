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

PERF_FUNC_ENTRY(translate_bytes,
                "Perform byte-translation lookups in 256-entry tables 64 at a time.", "wset", "tables", "rounds");
