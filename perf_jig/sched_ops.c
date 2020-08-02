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


typedef struct {
    u32 last_id;
    i32 balance;
} account_t;

typedef struct {
    u32 id;
    u32 from;
    u32 to;
    u32 delta;
} transfer_t;

static int perf_test_accounts_table(const char **args)
{
    char errbuf[1024] = {};
    seg_desc_t acct_seg = {.maplen = HUGE_2M_SIZE, .psize = HUGE_2M_SIZE, .flags = SEG_DESC_INITD | SEG_DESC_ANON};
    seg_desc_t acct_seg2 = {.maplen = HUGE_2M_SIZE, .psize = HUGE_2M_SIZE, .flags = SEG_DESC_INITD | SEG_DESC_ANON};
    seg_desc_t xfer_seg = {.maplen = HUGE_2M_SIZE * 4, .psize = HUGE_2M_SIZE, .flags = SEG_DESC_INITD | SEG_DESC_ANON};

    if (map_segment(NULL, &acct_seg, errbuf, sizeof(errbuf) - 1)) {
        printf("%s: %s\n", args[0], errbuf);
        return -1;
    }

    printf("%s\n", errbuf);

    if (map_segment(NULL, &acct_seg2, errbuf, sizeof(errbuf) - 1)) {
        unmap_segment(&acct_seg);
        printf("%s: %s\n", args[0], errbuf);
        return -1;
    }

    printf("%s\n", errbuf);

    if (map_segment(NULL, &xfer_seg, errbuf, sizeof(errbuf) - 1)) {
        unmap_segment(&acct_seg);
        unmap_segment(&acct_seg2);
        printf("%s: %s\n", args[0], errbuf);
        return -1;
    }

    printf("%s\n", errbuf);

    memset(acct_seg.ptr, 0, acct_seg.maplen);
    memset(acct_seg2.ptr, 0, acct_seg2.maplen);

    account_t * const RESTR account = (account_t *)acct_seg.ptr;
    account_t * const RESTR alt = (account_t *)acct_seg2.ptr;
    const u32 account_n = acct_seg.maplen / sizeof(account_t);
    const u32 account_mask = account_n - 1;

    if (account_n & account_mask) {
        printf("account_n must be a power of two and %u is not!\n", account_n);
        return -1;
    }

    transfer_t * const RESTR xfer = (transfer_t *)xfer_seg.ptr;

    const u32 xfer_n = xfer_seg.maplen / sizeof(transfer_t);
    randomize_data(xfer, xfer_seg.maplen);

    u32 i;

    for (i = 0; i < xfer_n; i++) {
        transfer_t * const RESTR x = xfer + i;
        x->id = i;
        x->from &= account_mask;
        x->to &= account_mask;
        x->to ^= (x->from == x->to);
        x->delta /= (x->delta > 0x10000) ? 0x10000 : 1;
    }

    const u64 pre_scalar = TSC_PRECISE();

    /* Process them one at a time on the alternate array to validate results. */
    for (i = 0; i < xfer_n; i++) {
        const transfer_t * const RESTR x = xfer + i;
        account_t * const RESTR from = alt + x->from;
        account_t * const RESTR to = alt + x->to;
        from->balance -= x->delta;
        to->balance += x->delta;
        from->last_id = x->id;
        to->last_id = x->id;
    }

    const u64 pre = TSC_PRECISE();

    i = 0;

    while (i < xfer_n) {
        const i64_8 idx = IDX_VEC(i64_8) + i;
        __mmask8 valid = VEC_TO_MASK(idx < xfer_n);
        u32 batchmax = __builtin_popcount(valid);

        mpv_8 x_batch = {.vec = ((idx * sizeof(transfer_t)) + (i64)xfer) | (idx >= xfer_n)};
        const u32_8 xid = GATHER_u32_8_FROM_STRUCTS(transfer_t, id, x_batch);
        const u32_8 from = GATHER_u32_8_FROM_STRUCTS(transfer_t, from, x_batch);
        const u32_8 to = GATHER_u32_8_FROM_STRUCTS(transfer_t, to, x_batch);

        const union {
            u32_16 zmm;
            struct {
                u32_8 from;
                u32_8 to;
            };
        } t0 = {.from = from, .to = to};
        const u32_8 delta = GATHER_u32_8_FROM_STRUCTS(transfer_t, delta, x_batch);
        const u32_16 t1 = __builtin_shuffle(t0.zmm, (const u32_16) {
            0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15
        });
        const u32_16 t2 = conflict_detect_u32_16(t1, ~(const u32_16) {
            0
        }, ~0);

        const __mmask16 conf = VEC_TO_MASK(t2 != 0);

        if (conf) {
            const u32 first = __builtin_ffs(conf);
            const u32 n_ok = (first - 1) / 2;
            const u32 ok_mask = (1 << n_ok) - 1;
            valid &= ok_mask;
            batchmax = (n_ok < batchmax) ? n_ok : batchmax;
        }

        const i64_8 from64 = (i64_8)_mm512_cvtepu32_epi64((__m256i)from);
        const i64_8 to64 = (i64_8)_mm512_cvtepu32_epi64((__m256i)to);
        const i64_8 inv64 = MASK_TO_VEC(~valid, i64_8);
        mpv_8 from_batch = {.vec = ((from64 * sizeof(account_t)) + (i64)account) | inv64};
        mpv_8 to_batch = {.vec = ((to64 * sizeof(account_t)) + (i64)account) | inv64};
        const i32_8 bal_from_old = (i32_8)GATHER_u32_8_FROM_STRUCTS(account_t, balance, from_batch);
        const i32_8 bal_to_old = (i32_8)GATHER_u32_8_FROM_STRUCTS(account_t, balance, to_batch);
        const i32_8 bal_from_new = bal_from_old - (i32_8)delta;
        const i32_8 bal_to_new = bal_to_old + (i32_8)delta;

        SCATTER_u32_8_TO_STRUCTS(account_t, balance, from_batch, bal_from_new);
        SCATTER_u32_8_TO_STRUCTS(account_t, balance, to_batch, bal_to_new);
        SCATTER_u32_8_TO_STRUCTS(account_t, last_id, from_batch, xid);
        SCATTER_u32_8_TO_STRUCTS(account_t, last_id, to_batch, xid);
        i += batchmax;
    }

    const u64 post = TSC_PRECISE();

    printf("%s: %u transactions processed in %lu clocks. (%.2f clocks/transaction).\n",
           args[0], xfer_n, post - pre, (float)(post - pre) / (float)xfer_n);

    printf("%s: Scalar reference took %.2f clocks/transaction.\n",
           args[0], (float)(pre - pre_scalar) / (float)xfer_n);

    if (memcmp(account, alt, acct_seg.maplen)) {
        printf("%s: Result validation failed!\n", args[0]);
        return -1;
    } else {
        printf("%s: Result validation OK.\n", args[0]);
    }

    return 0;
}

PERF_FUNC_ENTRY(accounts_table,
                "Use conflict detect operation to prevent data races in a simplified accounts table.");
