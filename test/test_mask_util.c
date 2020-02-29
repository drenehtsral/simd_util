#include <stdio.h>
#include <stdlib.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

#define PING_PONG_TEST(_m_in, _vec_type)                                                \
({                                                                                      \
    const typeof(_m_in) mtmp = (_m_in);                                                 \
    const _vec_type vtmp = MASK_TO_VEC(mtmp, _vec_type);                                \
    const typeof(mtmp) mout = VEC_TO_MASK(vtmp);                                        \
    if (mout != mtmp) {                                                                 \
        printf(OUT_PREFIX "%lu-bit mask --> %s --> mask failed!\n",                     \
                sizeof(mtmp) * 8, #_vec_type);                                          \
        printf(OUT_PREFIX OUT_PREFIX "input_mask (0x%llx) != output_mask (0x%llx)\n",   \
                (unsigned long long)mtmp, (unsigned long long)mout);                    \
        printf(OUT_PREFIX OUT_PREFIX);                                                  \
        debug_print_vec(vtmp, ~0);                                                      \
        return 1;                                                                       \
    }                                                                                   \
}) /* end of macro */

#define INDEX_TEST(_vec_type)                                                           \
({                                                                                      \
    const _vec_type itmp = IDX_VEC(_vec_type);                                          \
    const unsigned n = VEC_LANES(itmp);                                                 \
    unsigned i;                                                                         \
    for (i = 0; i < n; i++) {                                                           \
        if (itmp[i] != i) {                                                             \
            printf(OUT_PREFIX "IDX_VEC(%s) test failed!\n", #_vec_type);                \
            printf(OUT_PREFIX OUT_PREFIX);                                              \
            debug_print_vec(itmp, ~0);                                                  \
            return -1;                                                                  \
        }                                                                               \
    }                                                                                   \
}) /* end of macro */

#define _CHECK_SANITY(_expr, _str, _file, _line)                                    \
({                                                                                  \
    const int res = _expr;                                                          \
    if (!res) {                                                                     \
        printf(OUT_PREFIX "Sanity check assertion %s failed! (%s:%d)\n", _str,      \
                 _file, _line);                                                     \
        return 1;                                                                   \
    }                                                                               \
}) /*end of macro */

#define CHECK_SANITY(__expr)    _CHECK_SANITY((__expr), #__expr, __FILE__, __LINE__)

static union {
    u64     u64[1024];
    u32     u32[0];
    u32_16  z[0];
    u32_8   y[0];
} my_bit_table = {};

static int test_bit_table_lookup()
{
    const u64 nbits = sizeof(my_bit_table) * 8;
    u64 tmp[2] = {};
    u64 i = 0;

    /* Set some predictable bits. */
    while(i < nbits) {
        const u64 w = i / 64;
        const u64 b = i % 64;
        const u64 v = 1UL << b;
        my_bit_table.u64[w] |= v;

        tmp[0] = tmp[1];
        tmp[1] = i ? i : 1;
        i = tmp[0] + tmp[1];
    }

    const u64_8 idx_a = IDX_VEC(u64_8);
    const __mmask8 exp_a = 0x2F;
    const __mmask8 out_a = lookup_P2_bit_x8(my_bit_table.u64, nbits, idx_a);

    CHECK_SANITY(out_a == exp_a);

    const u32_16 idx_b = {
        0,      1,      2,      3,      5,      8,      13,         21,
        34,     55,     89,     144,    233,    377,    610,        987
    };
    const __mmask16 exp_b = 0xFFFF;
    const __mmask16 out_b = lookup_P2_bit_x16(my_bit_table.u32, nbits, idx_b);

    CHECK_SANITY(out_b == exp_b);

    const u32_16 idx_c = idx_b + 1;
    const __mmask16 exp_c = 0x0007;
    const __mmask16 out_c = lookup_P2_bit_x16(my_bit_table.u32, nbits, idx_c);

    CHECK_SANITY(out_c == exp_c);

    return 0;
}

int main(int argc, char **argv)
{
    const union {
        unsigned long long m64;
        unsigned       m32;
        unsigned short     m16;
        unsigned char      m8;
    } in = { .m64 =  0xfedcba9876543211ULL };


    PING_PONG_TEST(in.m64, u8_64);
    PING_PONG_TEST(in.m32, u8_32);
    PING_PONG_TEST(in.m16, u8_16);

    PING_PONG_TEST(in.m32, u16_32);
    PING_PONG_TEST(in.m16, u16_16);
    PING_PONG_TEST(in.m8, u16_8);

    PING_PONG_TEST(in.m16, u32_16);
    PING_PONG_TEST(in.m8, u32_8);
    PING_PONG_TEST((in.m8 & 0xF), u32_4);

    PING_PONG_TEST(in.m8, u64_8);
    PING_PONG_TEST(in.m8 & 0xF, u64_4);
    PING_PONG_TEST(in.m8 & 0x3, u64_2);

    INDEX_TEST(u8_64);
    INDEX_TEST(u16_32);
    INDEX_TEST(u32_16);
    INDEX_TEST(u64_8);

    INDEX_TEST(u8_32);
    INDEX_TEST(u16_16);
    INDEX_TEST(u32_8);
    INDEX_TEST(u64_4);

    INDEX_TEST(u8_16);
    INDEX_TEST(u16_8);
    INDEX_TEST(u32_4);
    INDEX_TEST(u64_2);

    if (test_bit_table_lookup()) {
        printf(OUT_PREFIX "%s FAIL!\n", __FILE__);
        return 1;
    }

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
