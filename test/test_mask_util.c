#include <stdio.h>
#include <stdlib.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

#define PING_PONG_TEST(_m_in, _vec_type)								\
({													\
    const typeof(_m_in) mtmp = (_m_in);									\
    const _vec_type vtmp = MASK_TO_VEC(mtmp, _vec_type);						\
    const typeof(mtmp) mout = VEC_TO_MASK(vtmp);							\
    if (mout != mtmp) {											\
        printf(OUT_PREFIX "%lu-bit mask --> %s --> mask failed!\n", sizeof(mtmp) * 8, #_vec_type);	\
        printf(OUT_PREFIX OUT_PREFIX "input_mask (0x%llx) != output_mask (0x%llx)\n",			\
                (unsigned long long)mtmp, (unsigned long long)mout);					\
        printf(OUT_PREFIX OUT_PREFIX);									\
        debug_print_vec(vtmp, ~0);									\
        return 1;											\
    }													\
}) /* end of macro */

#define INDEX_TEST(_vec_type)							\
({										\
    const _vec_type itmp = IDX_VEC(_vec_type);					\
    const unsigned n = VEC_LANES(itmp);						\
    unsigned i;									\
    for (i = 0; i < n; i++) {							\
        if (itmp[i] != i) {							\
            printf(OUT_PREFIX "IDX_VEC(%s) test failed!\n", #_vec_type);	\
            printf(OUT_PREFIX OUT_PREFIX);					\
            debug_print_vec(itmp, ~0);						\
            return -1;								\
        }									\
    }										\
}) /* end of macro */

int main(int argc, char **argv)
{
    const union {
        unsigned long long m64;
        unsigned	   m32;
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

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
