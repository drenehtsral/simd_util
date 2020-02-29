#include <stdio.h>
#include <stdlib.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

#define CHECK_ALIGNMENT_AND_SIZE(_entity, _expected, _line)                         \
({                                                                                  \
    const size_t expected = _expected;                                              \
    const size_t align = alignof(_entity);                                          \
    const size_t size = sizeof(_entity);                                            \
    if (align != expected) {                                                        \
        printf(OUT_PREFIX "Alignment of %s is %lu not the expected %lu!"            \
                " (%s:%d)\n", #_entity, align, expected,                            \
                __FILE__, _line);                                                   \
        return ++test_idx;                                                          \
    }                                                                               \
    if (size != expected) {                                                         \
        printf(OUT_PREFIX "Size of %s is %lu not the expected %lu!"                 \
                " (%s:%d)\n", #_entity, size, expected,                             \
                __FILE__, _line);                                                   \
        return ++test_idx;                                                          \
    }                                                                               \
}) /*end of macro */

#define _CHECK_SANITY(_expr, _str, _file, _line)                                    \
({                                                                                  \
    const int res = _expr;                                                          \
    if (!res) {                                                                     \
        printf(OUT_PREFIX "Sanity check assertion %s failed! (%s:%d)\n", _str,      \
                 _file, _line);                                                     \
        return ++test_idx;                                                          \
    }                                                                               \
}) /*end of macro */

#define CHECK_SANITY(__expr)    _CHECK_SANITY((__expr), #__expr, __FILE__, __LINE__)

int main(int argc, char **argv)
{
    int test_idx = 0;
    union {
        u32_4 x4;
        u32_8 x8;
        u32_16 x16;
        unsigned int s[16];
    } tmp_accum = {};

    const u32_8 a;
    const i64_8 b;
    CHECK_SANITY(IS_VEC_LEN(a, 32));
    CHECK_SANITY(!IS_VEC_LEN(test_idx, 16));
    CHECK_SANITY(IS_CONST(a));
    CHECK_SANITY(!IS_CONST(tmp_accum.x4));

    CHECK_SANITY(EXPR_MATCHES_TYPE(a[0], unsigned int));
    CHECK_SANITY(!EXPR_MATCHES_TYPE(a[0], unsigned long int));
    CHECK_SANITY(!EXPR_MATCHES_TYPE(a[0], signed int));

    CHECK_SANITY(EXPR_MATCHES_TYPE(b, i64_8));
    CHECK_SANITY(!EXPR_MATCHES_TYPE(b, u64_8));
    CHECK_SANITY(!EXPR_MATCHES_TYPE(b, i64_4));

    CHECK_SANITY(EXPR_TYPES_MATCH(a, a + 2));

    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x4, 16, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x8, 32, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x16, 64, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum, 64, __LINE__);

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
