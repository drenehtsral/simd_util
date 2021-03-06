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

static inline int validate_non_conflict(const u32 * const RESTR hash, const u32 n)
{
    int i, j;

    for (i = 1; i < n; i++) {
        for (j = 0; j < i; j++) {
            if (hash[i] == hash[j]) {
                return -1;
            }
        }
    }

    return 0;
}

/*
 * Validate the batch scheduler on fundamental constraints:
 *      == Each batch it returns must contain no internal conflicts on hash.
 *      == Every item that appears in the input queue must finally appear in an output batch
 *      == Positions for any given hash value must remain sorted in the output queue
 */
static int test_schedule_batch(void)
{
    u32 hash[] = {
        'A', 'B', 'C', 'A', 'D', 'E', 'F', 'G', 'B', 'H', 'I', 'J', 'A', 'K', 'C', 'D',
        'L', 'M', 'N', 'B', 'H', 'K'
    };
    const unsigned n = sizeof(hash) / sizeof(hash[0]);
    u32 posn[n];
    u32 posn_check[n];
    unsigned i;

    for (i = 0; i < n; i++) {
        posn[i] = i;
        posn_check[i] = 0;
    }

    // Construct batches ensuring that each batch meets non-conflict criteria
    for (i = 0; i < n; )
    {
        const int n_batch = schedule_batch(hash + i, posn + i, n - i);

        CHECK_SANITY(validate_non_conflict(hash + i, n_batch) == 0);
        i += n_batch;
    }

    // Tally counts for items in the rearranged queue.
    for (i = 0; i < n; i++) {
        posn_check[posn[i]]++;
    }

    // Ensure each item in the initial queue occurs exactly once in the rearranged queue
    for (i = 0; i < n; i++) {
        CHECK_SANITY(posn_check[i] == 1);
    }

    // Ensure each subset (by hash) remains sorted by position.  i.e. the Nth item with any given
    // hash will always be assigned to a batch before the (N+1)th item with that same hash.
    u32 target;

    for (target = 'A'; target <= 'N'; target++) {
        int prev = -1;

        for (i = 0; i < n; i++) {
            if (hash[i] == target) {
                if (prev < 0) {
                    prev = posn[i];
                } else {
                    CHECK_SANITY(prev < posn[i]);
                    prev = posn[i];
                }
            }
        }
    }

    return 0;
}

#define _TEST_MUX_BLEND(_type, _tstr, _file, _line)                                             \
({                                                                                              \
    const _type out = MUX_ON_MASK(in_mask.m64, in_true._type, in_false._type);                  \
    const unsigned nlanes = VEC_LANES(out);                                                     \
    const u64 mask_mask = (~0UL) >> (64 - nlanes);                                              \
    const u64 mout = VEC_TO_MASK(out) & mask_mask;                                              \
    if ((mout ^ in_mask.u64) & mask_mask) {                                                     \
        printf(OUT_PREFIX "Unexpected blend results for MUX_ON_MASK() with type %s. (%s:%d)\n", \
                _tstr, _file, _line);                                                           \
        debug_print_vec(in_true._type, ~0);                                                     \
        debug_print_vec(in_false._type, ~0);                                                    \
        debug_print_vec(out, ~0);                                                               \
        printf("Mask in: %#20lx  Mask out: %#20lx\n", in_mask.u64 & mask_mask, mout);           \
        return 1;                                                                               \
    }                                                                                           \
}) /* end of macro */

#define TEST_MUX_BLEND(__type) _TEST_MUX_BLEND(__type, #__type, __FILE__, __LINE__)

static int test_mux_blend(void)
{
    const union {
        __mmask8    m8;
        __mmask16   m16;
        __mmask32   m32;
        __mmask64   m64;
        u64         u64;
    } in_mask = { .m64 = 0xdeadbeef15f00d11UL };

    MEGA_UNION in_true, in_false;

    unsigned i;

    for (i = 0; i < sizeof(in_true); i++) {
        in_false.u8[i] = i & 0xFF;
        in_true.u8[i] = ~i & 0xFF;
    }

    TEST_MUX_BLEND(u8_16);
    TEST_MUX_BLEND(u8_32);
    TEST_MUX_BLEND(u8_64);

    TEST_MUX_BLEND(u16_8);
    TEST_MUX_BLEND(u16_16);
    TEST_MUX_BLEND(u16_32);

    TEST_MUX_BLEND(u32_4);
    TEST_MUX_BLEND(u32_8);
    TEST_MUX_BLEND(u32_16);

    TEST_MUX_BLEND(u64_2);
    TEST_MUX_BLEND(u64_4);
    TEST_MUX_BLEND(u64_8);

    TEST_MUX_BLEND(f32_4);
    TEST_MUX_BLEND(f32_8);
    TEST_MUX_BLEND(f32_16);

    TEST_MUX_BLEND(f64_2);
    TEST_MUX_BLEND(f64_4);
    TEST_MUX_BLEND(f64_8);
    return 0;
}

int main(int argc, char **argv)
{
    const union {
        unsigned long long  m64;
        unsigned            m32;
        unsigned short      m16;
        unsigned char       m8;
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

    if (test_mux_blend()) {
        printf(OUT_PREFIX "%s FAIL!\n", __FILE__);
        return 1;
    }

    if (test_schedule_batch()) {
        printf(OUT_PREFIX "%s FAIL!\n", __FILE__);
        return 1;
    }

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
