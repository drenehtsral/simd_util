#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

static void OPT_SIZE debug_transpose(const transpose_block_test_in_t *in,
                                     const transpose_block_test_out_t *out)
{
    debug_print_vec(in->z[0], ~0);
    debug_print_vec(in->z[1], ~0);
    debug_print_vec(in->z[2], ~0);
    debug_print_vec(in->z[3], ~0);
    debug_print_vec(in->z[4], ~0);
    debug_print_vec(in->z[5], ~0);
    debug_print_vec(in->z[6], ~0);
    debug_print_vec(in->z[7], ~0);
    debug_print_vec(out->y[0], ~0);
    debug_print_vec(out->y[1], ~0);
    debug_print_vec(out->y[2], ~0);
    debug_print_vec(out->y[3], ~0);
    debug_print_vec(out->y[4], ~0);
    debug_print_vec(out->y[5], ~0);
    debug_print_vec(out->y[6], ~0);
    debug_print_vec(out->y[7], ~0);
    debug_print_vec(out->y[8], ~0);
    debug_print_vec(out->y[9], ~0);
    debug_print_vec(out->y[10], ~0);
    debug_print_vec(out->y[11], ~0);
    debug_print_vec(out->y[12], ~0);
    debug_print_vec(out->y[13], ~0);
    debug_print_vec(out->y[14], ~0);
    debug_print_vec(out->y[15], ~0);
}

typedef union {
    u32_16 _z;
    struct {
        u32 a, b;
        u64 c[2];
        void *d;
        u32 e[8];
    };
} foobar_t;

typedef union {
    u64_8   _z64[8];
    u32_16  _z32[8];
    u32_8   _y32[16];
    struct {
        u32_8 a, b;
        u64_8 c[2];
        u64_8 d;
        u32_8 e[8];
    };
} foobar_x8_t;

int test_mixed_transpose(void)
{
    foobar_t in[8] = {};
    foobar_x8_t out;

    unsigned i;

    for (i = 0; i < 8; i++) {
        foobar_t tmp = {.a = 0, .b = 1, .c = {0x222200002222ul, 0x333300003333ul}, .d = (void *)0x444400004444ul, .e = {5, 6, 7, 8, 9, 10, 11, 12}};
        tmp._z |= (i << 28);
        in[i]._z = tmp._z;
    }

    transpose_func((transpose_block_test_in_t *)in, (transpose_block_test_out_t *)&out);

    out.c[0] = deinterleave_u32_16(out._z32[1]);
    out.c[1] = deinterleave_u32_16(out._z32[2]);
    out.d =  deinterleave_u32_16(out._z32[3]);

    /* XXX: Automate this test... */
    debug_print_vec(out.a, ~0);
    debug_print_vec(out.b, ~0);
    debug_print_vec(out.c[0], ~0);
    debug_print_vec(out.c[1], ~0);
    debug_print_vec(out.d, ~0);
    debug_print_vec(out.e[0], ~0);
    debug_print_vec(out.e[1], ~0);

    return 0;
}

int test_basic_transpose(void)
{
    transpose_block_test_in_t in;
    transpose_block_test_out_t out;
    unsigned i, j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 16; j++) {
            in.z[i][j] = (i << 8) | j;
        }
    }

    transpose_func(&in, &out);

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            if(out.y[i][j] != in.z[j][i]) {
                printf(OUT_PREFIX "%s: in[%u][%u] != out[%u][%u]\n", __FUNCTION__, j, i, i, j);
                debug_transpose(&in, &out);
                return -1;
            }
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (test_basic_transpose()) {
        return -1;
    }

    if (test_mixed_transpose()) {
        return -1;
    }

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
