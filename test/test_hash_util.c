#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

static int test_murmur3(void)
{
    const char *tests[8] = {
        "spamspam",
        "",
        "spam_spam",
        "spam_spam12",
        "SPAM!SPAM!SPAM!SPAM!",
        "Hello, world!",
        "Hello, world!",
        NULL
    };
    const u32_8 seed = {42, 1234, 42, 42, 42, 1234, 4321, 0};
    u32_8 len = {};
    u32_8 ref = {};
    mpv_8 tp = {};

    unsigned i;

    for (i = 0; i < 8; i++) {
        if (tests[i] != NULL) {
            len[i] = strlen(tests[i]);
            ref[i] = murmur3_u32(tests[i], len[i], seed[i]);
            tp.mp[i].cp = tests[i];
        }
    }

    const __mmask8 m = VEC_TO_MASK(tp.vec > 0);
    const u32_8 h = murmur3_u32_8(&tp, len, seed);
//    debug_print_vec(ref, m);
//    debug_print_vec(h, m);

    __mmask8 errmsk = VEC_TO_MASK(h != ref) & m;

    if (errmsk) {
        printf(OUT_PREFIX "%s Error: Disagreement between scalar and vector...\n", __FILE__);
        printf(OUT_PREFIX);
        debug_print_vec(ref, errmsk);
        printf(OUT_PREFIX);
        debug_print_vec(h, errmsk);
        return -1;
    }

    const __mmask8 m2 = m & VEC_TO_MASK((len & 3) == 0);
    tp.vec &= MASK_TO_VEC(m2, typeof(tp.vec));
    const u32_8 h_notail = murmur3_u32_8_notail(&tp, len / 4, seed);

    errmsk = VEC_TO_MASK(h_notail != ref) & m2;

//    debug_print_vec(h_notail, m2);

    if (errmsk) {
        printf(OUT_PREFIX "%s Error: Disagreement between scalar and vector...\n", __FILE__);
        printf(OUT_PREFIX);
        debug_print_vec(ref, errmsk);
        printf(OUT_PREFIX);
        debug_print_vec(h_notail, errmsk);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (test_murmur3()) {
        return 1;
    }

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
