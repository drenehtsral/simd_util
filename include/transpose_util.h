#ifndef _TRANSPOSE_UTIL_H
#define _TRANSPOSE_UTIL_H

/*
 * XXX: This whole file is experimantal and _badly_ needs a refactor but until I figure out
 * what direction(s) to take it in / how best to usefully generalize AOS-to-SOA and SOA-to-AOS
 * transformations for any given struct S within some basic/reasonable limits (fields all 32-bit or
 * 64-bit aligned) I'm not sure exactly what that'll look like.  So all these types, functions,
 * etc. are subject to change.
 */

typedef union {
    u32_16  z[8];
} transpose_block_test_in_t;

typedef union {
    u32_16  z[8];
    u32_8   y[16];
} transpose_block_test_out_t;

/*
 * About 40 clocks if well scheduled and data in cache.
 */
static inline void transpose_func(const transpose_block_test_in_t * const RESTR in,
                                  transpose_block_test_out_t * const RESTR out)
{
    const u32_16 idx0a = IDX_VEC_CUSTOM(u32_16,
                                        0x00, 0x10, 0x01, 0x11, 0x02, 0x12, 0x03, 0x13,
                                        0x04, 0x14, 0x05, 0x15, 0x06, 0x16, 0x07, 0x17);
    const u32_16 idx0b = idx0a + 8;
    const u32_16 a02 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[0], (__m512i)(idx0a),
                       (__m512i)in->z[2]);
    const u32_16 b02 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[0], (__m512i)(idx0b),
                       (__m512i)in->z[2]);
    const u32_16 a13 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[1], (__m512i)(idx0a),
                       (__m512i)in->z[3]);
    const u32_16 b13 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[1], (__m512i)(idx0b),
                       (__m512i)in->z[3]);
    const u32_16 a46 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[4], (__m512i)(idx0a),
                       (__m512i)in->z[6]);
    const u32_16 b46 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[4], (__m512i)(idx0b),
                       (__m512i)in->z[6]);
    const u32_16 a57 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[5], (__m512i)(idx0a),
                       (__m512i)in->z[7]);
    const u32_16 b57 = (u32_16) _mm512_permutex2var_epi32((__m512i)in->z[5], (__m512i)(idx0b),
                       (__m512i)in->z[7]);
    const u32_16 t0 = (u32_16) _mm512_permutex2var_epi32((__m512i)a02, (__m512i)(idx0a), (__m512i)a13);
    const u32_16 t1 = (u32_16) _mm512_permutex2var_epi32((__m512i)a02, (__m512i)(idx0b), (__m512i)a13);
    const u32_16 t2 = (u32_16) _mm512_permutex2var_epi32((__m512i)b02, (__m512i)(idx0a), (__m512i)b13);
    const u32_16 t3 = (u32_16) _mm512_permutex2var_epi32((__m512i)b02, (__m512i)(idx0b), (__m512i)b13);
    const u32_16 t4 = (u32_16) _mm512_permutex2var_epi32((__m512i)a46, (__m512i)(idx0a), (__m512i)a57);
    const u32_16 t5 = (u32_16) _mm512_permutex2var_epi32((__m512i)a46, (__m512i)(idx0b), (__m512i)a57);
    const u32_16 t6 = (u32_16) _mm512_permutex2var_epi32((__m512i)b46, (__m512i)(idx0a), (__m512i)b57);
    const u32_16 t7 = (u32_16) _mm512_permutex2var_epi32((__m512i)b46, (__m512i)(idx0b), (__m512i)b57);
    const u32_16 idx1a = IDX_VEC_CUSTOM(u32_16,
                                        0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13,
                                        0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17);
    const u32_16 idx1b = idx1a + 8;
    out->z[0] = (u32_16) _mm512_permutex2var_epi32((__m512i)t0, (__m512i)(idx1a), (__m512i)t4);
    out->z[1] = (u32_16) _mm512_permutex2var_epi32((__m512i)t0, (__m512i)(idx1b), (__m512i)t4);
    out->z[2] = (u32_16) _mm512_permutex2var_epi32((__m512i)t1, (__m512i)(idx1a), (__m512i)t5);
    out->z[3] = (u32_16) _mm512_permutex2var_epi32((__m512i)t1, (__m512i)(idx1b), (__m512i)t5);
    out->z[4] = (u32_16) _mm512_permutex2var_epi32((__m512i)t2, (__m512i)(idx1a), (__m512i)t6);
    out->z[5] = (u32_16) _mm512_permutex2var_epi32((__m512i)t2, (__m512i)(idx1b), (__m512i)t6);
    out->z[6] = (u32_16) _mm512_permutex2var_epi32((__m512i)t3, (__m512i)(idx1a), (__m512i)t7);
    out->z[7] = (u32_16) _mm512_permutex2var_epi32((__m512i)t3, (__m512i)(idx1b), (__m512i)t7);
}

static inline u64_8 deinterleave_u32_16(const u32_16 in)
{
    const u32_16 idx = IDX_VEC_CUSTOM(u32_16, 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);
    return (u64_8) _mm512_permutexvar_epi32((__m512i)idx, (__m512i)in);
}

#endif /* _TRANSPOSE_UTIL_H */
