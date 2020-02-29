#include <stdio.h>
#include <stdlib.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"


int main(int argc, char **argv)
{

    union {
        u8_16 x8;
        u8_32 y8;
        u8_64 z8;
        u16_8 x16;
        u16_16 y16;
        u16_32 z16;
        u32_4 x32;
        u32_8 y32;
        u32_16 z32;
        u64_2 x64;
        u64_4 y64;
        u64_8 z64;
    } test;

    unsigned i;

    for (i = 0; i < sizeof(test.z8); i++) test.z8[i] = i;

    FILE *f = fopen("/dev/null", "a+");

    if (f == NULL) {
        printf(OUT_PREFIX "Cannot open /dev/null, using stdout.\n");
        f = stdout;
    }

    debug_fprint_vec(f, test.x8, ~2);
    debug_fprint_vec(f, test.y8, ~4);
    debug_fprint_vec(f, test.z8, ~8);

    debug_fprint_vec(f, test.x16, ~2);
    debug_fprint_vec(f, test.y16, ~4);
    debug_fprint_vec(f, test.z16, ~8);

    debug_fprint_vec(f, test.x32, ~2);
    debug_fprint_vec(f, test.y32, ~4);
    debug_fprint_vec(f, test.z32, ~8);

    debug_fprint_vec(f, test.x64, ~2);
    debug_fprint_vec(f, test.y64, ~4);
    debug_fprint_vec(f, test.z64, ~8);

    if (f != stdout) {
        fclose(f);
    }

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
