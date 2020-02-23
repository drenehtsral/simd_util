#include <stdio.h>
#include <stdlib.h>

#include "../include/simd_util.h"

#define OUT_PREFIX "\t"

#define CHECK_ALIGNMENT_AND_SIZE(_entity, _expected, _line)     \
({                                  \
    const size_t expected = _expected;                  \
    const size_t align = alignof(_entity);              \
    const size_t size = sizeof(_entity);                \
    if (align != expected) {                        \
        printf(OUT_PREFIX "Alignment of %s is %lu not the expected %lu!"\
                " (%s:%d)\n", #_entity, align, expected,        \
                __FILE__, _line);                   \
        return ++test_idx;                      \
    }                                   \
    if (size != expected) {                     \
        printf(OUT_PREFIX "Size of %s is %lu not the expected %lu!" \
                " (%s:%d)\n", #_entity, size, expected,             \
                __FILE__, _line);                   \
        return ++test_idx;                      \
    }                                   \
}) /*end of macro */


int main(int argc, char **argv)
{
    int test_idx = 0;
    union {
        u32_4 x4;
        u32_8 x8;
        u32_16 x16;
        unsigned int s[16];
    } tmp_accum = {};

    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x4, 16, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x8, 32, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum.x16, 64, __LINE__);
    CHECK_ALIGNMENT_AND_SIZE(tmp_accum, 64, __LINE__);

    printf(OUT_PREFIX "%s: PASS\n", __FILE__);
    return 0;
}
