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

void usage(const char *prog)
{
    printf("Usage:\n\t%s test[:param[:param...]] ...\n\n", prog);
    printf("Where tests are as follows:\n");
    unsigned i;
    const perf_func_entry_t * trav;

    for (trav = &__start_test_desc_section; trav < &__stop_test_desc_section; trav++) {
        if (trav->func == NULL) {
            break;
        }

        printf("\t%s", trav->arg_names[0]);

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
        const perf_func_entry_t *trav;

        for (trav = &__start_test_desc_section; trav < &__stop_test_desc_section; trav++) {
            if (trav->func == NULL) {
                break;
            }

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
