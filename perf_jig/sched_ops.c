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

static int perf_test_schedule_batch(const char **args)
{
    const unsigned qlen     = ARG_VALID(args[1]) ? strtoul(args[1], NULL, 0) : 1024;
    const unsigned modulo   = ARG_VALID(args[2]) ? strtoul(args[2], NULL, 0) : 16;

    if (!qlen | !modulo) {
        printf("Queue length and modulo must both be greater than zero.\n");
        return -1;
    }

    u32 * const RESTR hash = (u32 *)malloc(qlen * sizeof(u32) * 2);

    if (hash == NULL) {
        printf("Cannot allocate queue of length %u\n", qlen);
        return -1;
    }

    u32 * const RESTR posn = hash + qlen;

    randomize_data(hash, qlen * sizeof(u32));

    unsigned i, batches = 0;

    for (i = 0; i < qlen; i++) {
        hash[i] %= modulo;
        posn[i] = i;
    }

    const u64 pre = TSC_PRECISE();

    for (i = 0; i < qlen; ) {
        const unsigned n = schedule_batch(hash + i, posn + i, qlen - i);
        i += n;
        batches++;
    }

    const u64 post = TSC_PRECISE();

    consume_data(hash, qlen * sizeof(u32) * 2);

    const u64 total_clk = post - pre;
    printf( "%s(%u, %u):\n\t%u batches in %lu cycles.\n"
            "\t(%.1f clocks per call, %.1f clocks per item).\n",
            args[0], qlen, modulo, batches, total_clk,
            (float)total_clk / (float)batches, (float)total_clk / (float)qlen);

    return 0;
}

PERF_FUNC_ENTRY(schedule_batch,
                "Perform batch scheduling operation", "qlen", "modulo");
