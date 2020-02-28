#ifndef _MASK_UTIL_H_
#define _MASK_UTIL_H_

static inline void __attribute__((__error__("Unknown vector size")))
_mask_util_unknown_vector_size(void)
{
    __builtin_unreachable();
}

static inline void __attribute__((__error__("Unknown lane size")))
_mask_util_unknown_lane_size(void)
{
    __builtin_unreachable();
}

/*
 * Supplied with an input mask (as a scalar) and a vector type this macro
 * returns a vector of the requested type where each lane has all bits
 * set or all bits clear (depending on the state of the correspondig bit
 * in the input mask).
 *
 * Any invocation of this entire lunking ~70 line macro should ultimately
 * compile down to *one single instruction* (since there are instructions
 * to broadcast Bit X to all bits of Lane X for each of the 12 fundamental
 * permutations (3 vector register sizes * 4 lane sizes), the rest is just
 * spinnach to make the compiler happy, suppress spurious warnings, etc.
 */
#define MASK_TO_VEC(_mask, _type)                                       \
({                                                                      \
    const unsigned long long __mask = (_mask);                          \
    union {                                                             \
        u8_16   x8;                                                     \
        u8_32   y8;                                                     \
        u8_64   z8;                                                     \
                                                                        \
        u16_8   x16;                                                    \
        u16_16  y16;                                                    \
        u16_32  z16;                                                    \
                                                                        \
        u32_4   x32;                                                    \
        u32_8   y32;                                                    \
        u32_16  z32;                                                    \
                                                                        \
        u64_2   x64;                                                    \
        u64_4   y64;                                                    \
        u64_8   z64;                                                    \
                                                                        \
        _type   out;                                                    \
    } _tmp = {};                                                        \
                                                                        \
    if (sizeof(_tmp.out) == 16) {                                       \
        /* Mask-->XMM register */                                       \
        if (sizeof(_tmp.out[0]) == 1) {                                 \
            _tmp.x8 = (u8_16) _mm_movm_epi8((__mmask16)__mask);         \
        } else if (sizeof(_tmp.out[0]) == 2) {                          \
            _tmp.x16 = (u16_8) _mm_movm_epi16((__mmask8)__mask);        \
        } else if (sizeof(_tmp.out[0]) == 4) {                          \
            _tmp.x32 = (u32_4) _mm_movm_epi32((__mmask8)__mask);        \
        } else if (sizeof(_tmp.out[0]) == 8) {                          \
            _tmp.x64 = (u64_2) _mm_movm_epi64((__mmask8)__mask);        \
        } else {                                                        \
            /* error case -- unknown lane size */                       \
            _mask_util_unknown_lane_size();                             \
        }                                                               \
    } else if (sizeof(_tmp.out) == 32) {                                \
        /* Mask-->YMM register */                                       \
        if (sizeof(_tmp.out[0]) == 1) {                                 \
            _tmp.y8 = (u8_32) _mm256_movm_epi8((__mmask32)__mask);      \
        } else if (sizeof(_tmp.out[0]) == 2) {                          \
            _tmp.y16 = (u16_16) _mm256_movm_epi16((__mmask16)__mask);   \
        } else if (sizeof(_tmp.out[0]) == 4) {                          \
            _tmp.y32 = (u32_8) _mm256_movm_epi32((__mmask8)__mask);     \
        } else if (sizeof(_tmp.out[0]) == 8) {                          \
            _tmp.y64 = (u64_4) _mm256_movm_epi64((__mmask8)__mask);     \
        } else {                                                        \
            /* error case -- unknown lane size */                       \
            _mask_util_unknown_lane_size();                             \
        }                                                               \
    } else if (sizeof(_tmp.out) == 64) {                                \
        /* Mask-->ZMM register */                                       \
        if (sizeof(_tmp.out[0]) == 1) {                                 \
            _tmp.z8 = (u8_64) _mm512_movm_epi8((__mmask64)__mask);      \
        } else if (sizeof(_tmp.out[0]) == 2) {                          \
            _tmp.z16 = (u16_32) _mm512_movm_epi16((__mmask32)__mask);   \
        } else if (sizeof(_tmp.out[0]) == 4) {                          \
            _tmp.z32 = (u32_16) _mm512_movm_epi32((__mmask16)__mask);   \
        } else if (sizeof(_tmp.out[0]) == 8) {                          \
            _tmp.z64 = (u64_8) _mm512_movm_epi64((__mmask8)__mask);     \
        } else {                                                        \
            /* error case -- unknown lane size */                       \
            _mask_util_unknown_lane_size();                             \
        }                                                               \
    } else {                                                            \
        /* error case -- unknown vector size */                         \
        _mask_util_unknown_vector_size();                               \
    }                                                                   \
    /* Return requested type */                                         \
    _tmp.out;                                                           \
}) /* end of macro */


/*
 * This macro takes any SIMD type and returns a zero-padded mask composed
 * such that bit X of the returned mask is the MSB of lane X of the input
 * vector for all X (0..(NLANES-1))
 *
 * Any invocation of this entire lunking ~70 line macro should ultimately
 * compile down to *one single instruction* (since there are instructions
 * to make a scalar from the gathered MSBs for all of the 12 fundamental
 * permutations (3 vector register sizes * 4 lane sizes), the rest is just
 * spinnach to make the compiler happy, suppress spurious warnings, etc.
 */
#define VEC_TO_MASK(_vec)                                   \
({                                                          \
    unsigned long long __mask = 0;                          \
    const union {                                           \
        __m128i x;                                          \
        __m256i y;                                          \
        __m512i z;                                          \
        typeof(_vec)    in;                                 \
    } _tmp = { .in = (_vec)};                               \
                                                            \
    if (sizeof(_tmp.in) == 16) {                            \
        /* XMM register --> Mask */                         \
        if (sizeof(_tmp.in[0]) == 1) {                      \
            __mask = _mm_movepi8_mask(_tmp.x);              \
        } else if (sizeof(_tmp.in[0]) == 2) {               \
            __mask = _mm_movepi16_mask(_tmp.x);             \
        } else if (sizeof(_tmp.in[0]) == 4) {               \
            __mask = _mm_movepi32_mask(_tmp.x);             \
        } else if (sizeof(_tmp.in[0]) == 8) {               \
            __mask = _mm_movepi64_mask(_tmp.x);             \
        } else {                                            \
            /* error case -- unknown lane size */           \
            _mask_util_unknown_lane_size();                 \
        }                                                   \
    } else if (sizeof(_tmp.in) == 32) {                     \
        /* YMM register --> Mask */                         \
        if (sizeof(_tmp.in[0]) == 1) {                      \
            __mask = _mm256_movepi8_mask(_tmp.y);           \
        } else if (sizeof(_tmp.in[0]) == 2) {               \
            __mask = _mm256_movepi16_mask(_tmp.y);          \
        } else if (sizeof(_tmp.in[0]) == 4) {               \
            __mask = _mm256_movepi32_mask(_tmp.y);          \
        } else if (sizeof(_tmp.in[0]) == 8) {               \
            __mask = _mm256_movepi64_mask(_tmp.y);          \
        } else {                                            \
            /* error case -- unknown lane size */           \
            _mask_util_unknown_lane_size();                 \
        }                                                   \
    } else if (sizeof(_tmp.in) == 64) {                     \
        /* ZMM register --> Mask */                         \
        if (sizeof(_tmp.in[0]) == 1) {                      \
            __mask = _mm512_movepi8_mask(_tmp.z);           \
        } else if (sizeof(_tmp.in[0]) == 2) {               \
            __mask = _mm512_movepi16_mask(_tmp.z);          \
        } else if (sizeof(_tmp.in[0]) == 4) {               \
            __mask = _mm512_movepi32_mask(_tmp.z);          \
        } else if (sizeof(_tmp.in[0]) == 8) {               \
            __mask = _mm512_movepi64_mask(_tmp.z);          \
        } else {                                            \
            /* error case -- unknown lane size */           \
            _mask_util_unknown_lane_size();                 \
        }                                                   \
    } else {                                                \
        /* error case -- unknown vector size */             \
        _mask_util_unknown_vector_size();                   \
    }                                                       \
    /* Return resulting mask */                             \
    __mask;                                                 \
}) /* end of macro */


static const union {
    __m128i x;
    __m256i y;
    __m512i z;
    unsigned char u8[64];
} __idx_u8_union = { .u8 = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    }
};

/*
 * While not *strictly* mask-related this macro to generate one of the
 * many zero-extend-all-lanes instructions allows a single source union
 * (above) to allow quick-and-inexpensive generation of a vector of the
 * form where each lane contains its own lane index.  Otherwise the
 * compiler will happily stick one in the initialized constants secion of
 * each object for each vector type, which is wasteful of cache, and bandwidth
 * (it's silly to store a u64_8 of {0, 1, 2, 3, 4, 5, 6, 7} using 64 bytes
 * of memory when you can store less.
 */
#define IDX_VEC(_type)                                          \
({                                                              \
    union {                                                     \
        __m128i x;                                              \
        __m256i y;                                              \
        __m512i z;                                              \
        _type out;                                              \
    } _tmp = {};                                                \
                                                                \
    if (sizeof(_tmp.out) == 16) {                               \
        /* XMM register --> Mask */                             \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.x = __idx_u8_union.x;                          \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.x = _mm_cvtepu8_epi16(__idx_u8_union.x);       \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.x = _mm_cvtepu8_epi32(__idx_u8_union.x);       \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.x = _mm_cvtepu8_epi64(__idx_u8_union.x);       \
        } else {                                                \
            /* error case -- unknown lane size */               \
            _mask_util_unknown_lane_size();                     \
        }                                                       \
    } else if (sizeof(_tmp.out) == 32) {                        \
        /* YMM register --> Mask */                             \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.y = __idx_u8_union.y;                          \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.y = _mm256_cvtepu8_epi16(__idx_u8_union.x);    \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.y = _mm256_cvtepu8_epi32(__idx_u8_union.x);    \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.y = _mm256_cvtepu8_epi64(__idx_u8_union.x);    \
        } else {                                                \
            /* error case -- unknown lane size */               \
            _mask_util_unknown_lane_size();                     \
        }                                                       \
    } else if (sizeof(_tmp.out) == 64) {                        \
        /* ZMM register --> Mask */                             \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.z = __idx_u8_union.z;                          \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.z = _mm512_cvtepu8_epi16(__idx_u8_union.y);    \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.z = _mm512_cvtepu8_epi32(__idx_u8_union.x);    \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.z = _mm512_cvtepu8_epi64(__idx_u8_union.x);    \
        } else {                                                \
            /* error case -- unknown lane size */               \
            _mask_util_unknown_lane_size();                     \
        }                                                       \
    } else {                                                    \
        /* error case -- unknown vector size */                 \
        _mask_util_unknown_vector_size();                       \
    }                                                           \
    /* Return resulting index vector */                         \
    _tmp.out;                                                   \
}) /* end of macro */

CONST_FUNC static inline __mmask8 lookup_256_bit_x8(const u32_8 table, const u32_8 idx_in)
{
    const u32_8 idx = idx_in & 0xFF;
    const u32_8 sel = idx >> 5;
    const u32_8 shift = 0x1F - (idx & 0x1F);
    const u32_8 t0 = (u32_8)_mm256_permutevar8x32_epi32((__m256i)table, (__m256i)sel);
    const u32_8 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

CONST_FUNC static inline __mmask16 lookup_512_bit_x16(const u32_16 table, const u32_16 idx_in)
{
    const u32_16 idx = idx_in & 0x1FF;
    const u32_16 sel = idx >> 5;
    const u32_16 shift = 0x1F - (idx & 0x1F);
    const u32_16 t0 = (u32_16)_mm512_permutexvar_epi32((__m512i)table, (__m512i)sel);
    const u32_16 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

CONST_FUNC static inline __mmask16 lookup_1024_bit_x16(const u32_16 tbl_lo, const u32_16 tbl_hi,
        const u32_16 idx_in)
{
    const u32_16 idx = idx_in & 0x3FF;
    const u32_16 sel = idx >> 5;
    const u32_16 shift = 0x1F - (idx & 0x1F);
    const u32_16 t0 = (u32_16)_mm512_permutex2var_epi32((__m512i)tbl_lo, (__m512i)sel, (__m512i)tbl_hi);
    const u32_16 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

#endif /* _MASK_UTIL_H_ */
