#ifndef _MASK_UTIL_H_
#define _MASK_UTIL_H_

extern int __attribute__((__error__("Unknown vector size"))) _mask_util_unknown_vector_size(void);

extern int __attribute__((__error__("Unknown lane size"))) _mask_util_unknown_lane_size(void);

extern int __attribute__((__error__("Mismatched types for mux/blend"))) _mismatched_types_for_mux(
        void);

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

/*
 *    This single-cache-line constant is useful because it can (using the IDX_VEC macro defined
 * below) replace in a single instruction what the compiler will otherwise generate as index vectors
 * of each type for which you need one, for each function in which you need one.  It's silly to
 * devote an entire cache line of binary space (and cache space) for each function that needs to
 * generate a constant u64_8 value of {0, 1, 2, 3, 4, 5, 6, 7} for instance.
 *    Since all such sites can zero-extend this constant out to any lane size and count permutation
 * it makes more sense to store this once and use it throughout the program rather than constructing
 * an address to load a distinct constant vector for each place such an index is needed.
 */
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
 * While not *strictly* mask-related this macro to generate one of the many zero-extend-all-lanes
 * instructions allows a single source union (above) to allow quick-and-inexpensive generation of a
 * vector of the form where each lane contains its own lane index.  Otherwise the compiler will
 * happily stick one in the initialized constants secion of each object (or even function) for each
 * vector type which is wasteful of cache and memory bandwidth.
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
        /* lane_nums --> XMM register */                        \
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
        /* lane_nums --> YMM register */                        \
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
        /* lane_nums --> ZMM register */                        \
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

/*
 * This is really a [slight] generalization of the above, only in that you can give it any u8_64
 * vector to zero-extend into the specified type.  This can be useful if you need some other pattern
 * of 8 least significant bits per lane.
 *
 * XXX: The compiler is not always smart enough to realize that if your input is only 8 values long
 * that it doesn't need to allocate an entire u64_8 worth of space, so this macro may actually not
 * really save you anything over reserving a whole u64_8 after all.  It may get smarter in time
 * though so encoding this intent may still have value in the long run.
 */
#define IDX_VEC_CUSTOM(_type, _vals...)                         \
({                                                              \
    static const typeof(__idx_u8_union) _in = {.u8 = {_vals}};  \
    union {                                                     \
        __m128i x;                                              \
        __m256i y;                                              \
        __m512i z;                                              \
        _type out;                                              \
    } _tmp = {};                                                \
                                                                \
    if (sizeof(_tmp.out) == 16) {                               \
        /* lane_nums --> XMM register */                        \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.x = _in.x;                                     \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.x = _mm_cvtepu8_epi16(_in.x);                  \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.x = _mm_cvtepu8_epi32(_in.x);                  \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.x = _mm_cvtepu8_epi64(_in.x);                  \
        } else {                                                \
            /* error case -- unknown lane size */               \
            _mask_util_unknown_lane_size();                     \
        }                                                       \
    } else if (sizeof(_tmp.out) == 32) {                        \
        /* lane_nums --> YMM register */                        \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.y = _in.y;                                     \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.y = _mm256_cvtepu8_epi16(_in.x);               \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.y = _mm256_cvtepu8_epi32(_in.x);               \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.y = _mm256_cvtepu8_epi64(_in.x);               \
        } else {                                                \
            /* error case -- unknown lane size */               \
            _mask_util_unknown_lane_size();                     \
        }                                                       \
    } else if (sizeof(_tmp.out) == 64) {                        \
        /* lane_nums --> ZMM register */                        \
        if (sizeof(_tmp.out[0]) == 1) {                         \
            _tmp.z = _in.z;                                     \
        } else if (sizeof(_tmp.out[0]) == 2) {                  \
            _tmp.z = _mm512_cvtepu8_epi16(_in.y);               \
        } else if (sizeof(_tmp.out[0]) == 4) {                  \
            _tmp.z = _mm512_cvtepu8_epi32(_in.x);               \
        } else if (sizeof(_tmp.out[0]) == 8) {                  \
            _tmp.z = _mm512_cvtepu8_epi64(_in.x);               \
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

/*
 * Look up 8 requested bits from a 256 bit table. High bits of idx_in are ignored.
 */
CONST_FUNC static inline __mmask8 lookup_256_bit_x8(const u32_8 table, const u32_8 idx_in)
{
    const u32_8 idx = idx_in & 0xFF;
    const u32_8 sel = idx >> 5;
    const u32_8 shift = 0x1F - (idx & 0x1F);
    const u32_8 t0 = (u32_8)_mm256_permutevar8x32_epi32((__m256i)table, (__m256i)sel);
    const u32_8 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

/*
 * Look up 16 requested bits from a 512 bit table. High bits of idx_in are ignored.
 */
CONST_FUNC static inline __mmask16 lookup_512_bit_x16(const u32_16 table, const u32_16 idx_in)
{
    const u32_16 idx = idx_in & 0x1FF;
    const u32_16 sel = idx >> 5;
    const u32_16 shift = 0x1F - (idx & 0x1F);
    const u32_16 t0 = (u32_16)_mm512_permutexvar_epi32((__m512i)table, (__m512i)sel);
    const u32_16 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

/*
 * Look up 16 requested bits from a 1024 bit table. High bits of idx_in are ignored.
 */
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

/*
 * Perform 8 parallel bit lookups from a table of tblbits bits (where tblbits % 64 == 0).  For any
 * lane in idx_in whose value exceeds (tblbits & ~63), 0 will be returned.  The return value of this
 * function is an 8 bit mask.  (Glomming together the 8 bits from the table specified by idx_in).
 */
PURE_FUNC static inline __mmask8 lookup_P2_bit_x8(const u64 * const RESTR table,
        const u64 tblbits, const u64_8 idx_in)
{
    const __m512i zero = {};
    const u64 ub = tblbits / BIT_SIZE(*table);
    const u64_8 tidx = idx_in / BIT_SIZE(*table);
    const __mmask8 gm = VEC_TO_MASK(tidx < ub);
    const u64_8 shift = (BIT_SIZE(*table) - 1) - (idx_in & (BIT_SIZE(*table) - 1));
    const u64_8 t0 = (u64_8)_mm512_mask_i64gather_epi64(zero, gm, (__m512i)tidx, table, sizeof(*table));
    const u64_8 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

/*
 * Perform 16 parallel bit lookups from a table of tblbits bits (where tblbits % 32 == 0).  For any
 * lane in idx_in whose value exceeds (tblbits & ~31), 0 will be returned.  The return value of this
 * function is a 16 bit mask.  (Glomming together the 16 bits from the table specified by idx_in).
 */
PURE_FUNC static inline __mmask16 lookup_P2_bit_x16(const u32 * const RESTR table,
        const u32 tblbits, const u32_16 idx_in)
{
    const __m512i zero = {};
    const u32 ub = tblbits / BIT_SIZE(*table);
    const u32_16 tidx = idx_in / BIT_SIZE(*table);
    const __mmask16 gm = VEC_TO_MASK(tidx < ub);
    const u32_16 shift = (BIT_SIZE(*table) - 1) - (idx_in & (BIT_SIZE(*table) - 1));
    const u32_16 t0 = (u32_16)_mm512_mask_i32gather_epi32(zero, gm, (__m512i)tidx, table,
                      sizeof(*table));
    const u32_16 t1 = t0 << shift;
    return VEC_TO_MASK(t1);
}

/*
 * This trivial wrapper around the vpconflictd instruction on zmm regs uses its zero-invalid-lanes form
 * and then blends the result with the inv_lane_vals input (rather than using the mask-but-don't-zero
 * form which forms a long dependency chain -- unless the inv_lane_vals input is waiting on a load
 * from farther away than L2 cache it'll be valid before the result of the vpconflictd instruction
 * anyway.  If you use the non-zeroing form you have to wait for the two things in series.
 */
CONST_FUNC static inline u32_16 conflict_detect_u32_16( const u32_16 data,
        const u32_16 inv_lane_vals,
        const __mmask16 lanes)
{
    const __m512i t0 = _mm512_maskz_conflict_epi32(lanes, (__m512i)data);
    const u32_16 t1 = (u32_16)_mm512_mask_blend_epi32(lanes, (__m512i)inv_lane_vals, t0);
    return t1;
}

/* Do the same for vpconflictq on zmm regs */
CONST_FUNC static inline u64_8 conflict_detect_u64_8(   const u64_8 data,
        const u64_8 inv_lane_vals,
        const __mmask8 lanes)
{
    const __m512i t0 = _mm512_maskz_conflict_epi64(lanes, (__m512i)data);
    const u64_8 t1 = (u64_8)_mm512_mask_blend_epi64(lanes, (__m512i)inv_lane_vals, t0);
    return t1;
}

/* Do the same for vpconflictd on ymm regs */
CONST_FUNC static inline u32_8 conflict_detect_u32_8(   const u32_8 data,
        const u32_8 inv_lane_vals,
        const __mmask8 lanes)
{
    const __m256i t0 = _mm256_maskz_conflict_epi32(lanes, (__m256i)data);
    const u32_8 t1 = (u32_8)_mm256_mask_blend_epi32(lanes, (__m256i)inv_lane_vals, t0);
    return t1;
}
/*
 * This function is a greedy (limited look-ahead) batch scheduling function for dispatching
 * batches of work in situations where there is a read-modify-write component such that it is
 * much more efficient to process work items in parallel if you know in advance that no two
 * work items in a batch refer to the same state machine instance.  This function swizzles
 * work items identified by hash (erring on the side of caution) and similarly swizzles
 * a second array (posn) used for (for instance) keeping track of the position in an input
 * FIFO of the event to be processed.
 *
 * The basic approach is to examine the first 16 items in the queue for hash collisions in an
 * attempt to assemble a batch of 8 without.  Any non-colliding items will be packed to the
 * beginning of the queue (preserving order within the set of non-colliding items) and any
 * colliding items will be packed to immediately after the last non-colliding item (maintaining
 * order within the set of colliding items) and the function will return the number of non-colliding
 * items that may be concurrently processed (up to 8).
 *
 * Within the subset defined by any given hash value (e.g. hash == 1234) order is maintained so that
 * events targeting the state machine instances assigned to any given hash will be processed in
 * order with respect to one another even if they are re-ordered with respect to the events for
 * other instances.
 *
 * Example:
 * u32 posn[max_n];
 * u32 hash[max_n];
 * u32 n = get_some_events(hash, posn, max_n);
 * for (i = 0; i < n; ) {
 *     const int batch_n = schedule_batch(hash + i, posn + i, n - i);
 *     const __mmask8 k = (1 << batch_n) - 1;
 *     process_event_x8(events, hash + i, posn + i, k);
 *     i += batch_n;
 * }
 *
 * Using this algorithm can have the effect of punting some events down the line if there are long
 * runs of contiguous hash collisions (e.g. A,A,A,A,B,C,D,E, ... where the 4th A will be processed
 * on the 4th batch (in order among the A subset, but far out of order with respect of other hashes)
 * so it's recommended to set an upper limit beyond which to flush straggling small batches of
 * deferred conflicts (e.g. break off N events, schedule them and execute until they're all done,
 * then break off another N events to keep from deferring long runs too far forward).
 */
static inline int schedule_batch(u32 * const RESTR hash, u32 * const RESTR posn, const u32 extent)
{
    // Constrain the extent to one 16-lane vector (a lookahead of 2x the 8-lane target)
    const u32 etmp = (extent < 16) ? extent : 16;
    const __mmask16 em = (1U << etmp) - 1;
    // Load the head chunk of both hash and position queues, careful re: extent
    const u32_16 h0 = (u32_16)_mm512_maskz_loadu_epi32(em, hash);
    const u32_16 p0 = (u32_16)_mm512_maskz_loadu_epi32(em, posn);
    // Compute conflict detection across the hashes in that chunk
    const u32_16 t0 = (u32_16)_mm512_maskz_conflict_epi32(em, (__m512i)h0);
    const u32_16 zero = {};
    const u32_16 idxvec = IDX_VEC(u32_16);
    // Extract a mask of non-conflicting lanes to pack to the head
    const __mmask16 m0 = _mm512_mask_cmpeq_epi32_mask(em, (__m512i)t0, (__m512i)zero);
    // And count them...
    const int n_ok = __builtin_popcount(m0);
    // Pack lane index for non-conflicting lanes to the head (low) end of a vector
    const u32_16 head = (u32_16)_mm512_maskz_compress_epi32(m0, (__m512i)idxvec);
    // Similarly pack lane index for conflicting lanes to the head of another vector
    const u32_16 tail = (u32_16)_mm512_maskz_compress_epi32(~m0, (__m512i)idxvec);
    // Compute a mask representing the spaces to fill at the end of head
    const __mmask16 xm = ~((1u << n_ok) - 1);
    // Append tail to head to yield a swizzle map of the desired rearrangement
    const u32_16 sw = (u32_16)_mm512_mask_expand_epi32((__m512i)head, xm, (__m512i)tail);
    // Apply that swizzle map to both the hash and position vectors
    // and store them back to the arrays from whence they came
    const u32_16 h1 = (u32_16)_mm512_permutexvar_epi32((__m512i)sw, (__m512i)h0);
    _mm512_mask_storeu_epi32(hash, em, (__m512i)h1);
    const u32_16 p1 = (u32_16)_mm512_permutexvar_epi32((__m512i)sw, (__m512i)p0);
    _mm512_mask_storeu_epi32(posn, em, (__m512i)p1);
    // Return safe batch size
    return (n_ok < 8) ? n_ok : 8;
}

/*
 * This macro is effectively a SIMD multiplexing function not unlike the ?: operation, except
 * that both the true and false inputs are evaluated unconditionally.  Each lane of the returned
 * vector will receive the value of the corresponding lane from either the _true or _false input
 * depending on the corresponding bit from the mask input.
 */
#define MUX_ON_MASK(_mask, _true, _false)                                                   \
({                                                                                          \
    const union {                                                                           \
        __m128i     x;                                                                      \
        __m256i     y;                                                                      \
        __m512i     z;                                                                      \
        typeof(_true) in;                                                                   \
    } _t_case = { .in = (_true)};                                                           \
                                                                                            \
    const union {                                                                           \
        __m128i     x;                                                                      \
        __m256i     y;                                                                      \
        __m512i     z;                                                                      \
        typeof(_false) in;                                                                  \
    } _f_case = { .in = (_false)};                                                          \
                                                                                            \
    const union {                                                                           \
        __mmask8    m8;                                                                     \
        __mmask16   m16;                                                                    \
        __mmask32   m32;                                                                    \
        __mmask64   m64;                                                                    \
        typeof(_mask) in;                                                                   \
    } _m_in = { .in = (_mask) };                                                            \
                                                                                            \
    union {                                                                                 \
        __m128i     x;                                                                      \
        __m256i     y;                                                                      \
        __m512i     z;                                                                      \
        typeof(_t_case.in) out;                                                             \
    } _ret = {};                                                                            \
                                                                                            \
    if (!EXPR_TYPES_MATCH(_t_case.in, _f_case.in)) {                                        \
        _mismatched_types_for_mux(); /* Balk if true and false types don't match. */        \
    } else {                                                                                \
                                                                                            \
        if (IS_VEC_LEN(_ret.out, 16)) {                                                     \
            /* Handle XMM register blends. */                                               \
            if (IS_LANE_SIZE(_ret.out, 1)) {                                                \
                _ret.x = _mm_mask_blend_epi8(_m_in.m16, _f_case.x, _t_case.x);              \
            } else if (IS_LANE_SIZE(_ret.out, 2)) {                                         \
                _ret.x = _mm_mask_blend_epi16(_m_in.m8, _f_case.x, _t_case.x);              \
            } else if (IS_LANE_SIZE(_ret.out, 4)) {                                         \
                _ret.x = _mm_mask_blend_epi32(_m_in.m8, _f_case.x, _t_case.x);              \
            } else if (IS_LANE_SIZE(_ret.out, 8)) {                                         \
                _ret.x = _mm_mask_blend_epi64(_m_in.m8, _f_case.x, _t_case.x);              \
            } else {                                                                        \
                _mask_util_unknown_lane_size();                                             \
            }                                                                               \
                                                                                            \
        } else if (IS_VEC_LEN(_ret.out, 32)) {                                              \
            /* Handle YMM register blends. */                                               \
            if (IS_LANE_SIZE(_ret.out, 1)) {                                                \
                _ret.y = _mm256_mask_blend_epi8(_m_in.m32, _f_case.y, _t_case.y);           \
            } else if (IS_LANE_SIZE(_ret.out, 2)) {                                         \
                _ret.y = _mm256_mask_blend_epi16(_m_in.m16, _f_case.y, _t_case.y);          \
            } else if (IS_LANE_SIZE(_ret.out, 4)) {                                         \
                _ret.y = _mm256_mask_blend_epi32(_m_in.m8, _f_case.y, _t_case.y);           \
            } else if (IS_LANE_SIZE(_ret.out, 8)) {                                         \
                _ret.y = _mm256_mask_blend_epi64(_m_in.m8, _f_case.y, _t_case.y);           \
            } else {                                                                        \
                _mask_util_unknown_lane_size();                                             \
            }                                                                               \
                                                                                            \
        } else if (IS_VEC_LEN(_ret.out, 64)) {                                              \
            /* Handle ZMM register blends. */                                               \
            if (IS_LANE_SIZE(_ret.out, 1)) {                                                \
                _ret.z = _mm512_mask_blend_epi8(_m_in.m64, _f_case.z, _t_case.z);           \
            } else if (IS_LANE_SIZE(_ret.out, 2)) {                                         \
                _ret.z = _mm512_mask_blend_epi16(_m_in.m32, _f_case.z, _t_case.z);          \
            } else if (IS_LANE_SIZE(_ret.out, 4)) {                                         \
                _ret.z = _mm512_mask_blend_epi32(_m_in.m16, _f_case.z, _t_case.z);          \
            } else if (IS_LANE_SIZE(_ret.out, 8)) {                                         \
                _ret.z = _mm512_mask_blend_epi64(_m_in.m8, _f_case.z, _t_case.z);           \
            } else {                                                                        \
                _mask_util_unknown_lane_size();                                             \
            }                                                                               \
        } else {                                                                            \
            _mask_util_unknown_vector_size();                                               \
        }                                                                                   \
    }                                                                                       \
    /* Return requested type */                                                             \
    _ret.out;                                                                               \
}) /* end of macro */

#endif /* _MASK_UTIL_H_ */
