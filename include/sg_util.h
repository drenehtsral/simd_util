#ifndef _SG_UTIL_H_
#define _SG_UTIL_H_

/*
 * Types for masked-pointers and maked-pointer-vectors.  The idea being that
 * bit 63 is used as an "invalid" flag while bits 62:0 still function as
 * a normal pointer (really, for now at least, it's safe to assume that we've
 * got a useful userland VA space <= 63 bits... These can pack along an implicit
 * mask to a gather macro (which can normalize the mask by setting any
 * lanes which (treated as signed, as the gather instruction takes) would be
 * <= zero (NULL, or already flagged invalid) and flags them invalid.
 */

typedef union {
    void *p;
    const void *cp;
    struct {
        u64 addr : 63,
            inv  : 1;
    };
} masked_ptr_u;

typedef union {
    i64_4       vec;
    masked_ptr_u    mp[4];
} mpv_4;

typedef union {
    i64_8       vec;
    masked_ptr_u    mp[8];
    mpv_4   v4[2];
} mpv_8;


/*
 * This little inline function takes a pointer to an mpv_4 masked pointer vector
 * and it updates the "invalid" bit (bit 63 of each pointer lane) for any lane
 * which already has bit 63 set (i.e. would be a negative offset with gather)
 * or which equals an explicit NULL pointer (0).  Treating the pointer vector
 * as a vector of 64 bit signed integers (like the gather instructions want)
 * this test is equivalent to:
 *    tmp = (lane <= 0) ? (1ULL << 63) : 0;
 *    lane |= tmp;
 *
 * This function also returns the inverted sense of the invalid bit (a mask of
 * valid lanes such as you might use with the gather instructions).
 */
static inline __mmask8 mpv_4_fixup_mask(mpv_4 * const RESTR pv)
{
    typeof(pv->vec) t0 = (pv->vec <= 0) & ((i64)MSB64);
    pv->vec |= t0;
    return (__mmask8) (VEC_TO_MASK(t0) ^ 0xF);
}

/*
 * Same as above but for 8-lane (zmm register) masked pointer vectors.
 */
static inline __mmask8 mpv_8_fixup_mask(mpv_8 * const RESTR pv)
{
    typeof(pv->vec) t0 = (pv->vec <= 0) & ((i64)MSB64);
    pv->vec |= t0;
    return (__mmask8) (VEC_TO_MASK(t0) ^ 0xFF);
}

#define GATHER_u32_4_FROM_MPV4(_mpv)                    \
({                                                      \
    const __m128i __zero = {};                          \
    mpv_4 __mpv = { .vec = (_mpv).vec };                \
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);   \
    (u32_4)_mm256_mmask_i64gather_epi32(__zero, __mask, \
            (__m256i)__mpv.vec, (void *)0, 1);          \
}) /* end of macro */

#define GATHER_u64_4_FROM_MPV4(_mpv)                    \
({                                                      \
    const __m256i __zero = {};                          \
    mpv_4 __mpv = { .vec = (_mpv).vec };                \
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);   \
    (u64_4)_mm256_mmask_i64gather_epi64(__zero, __mask, \
            (__m256i)__mpv.vec, (void *)0, 1);          \
}) /* end of macro */

#define GATHER_u32_8_FROM_MPV8(_mpv)                    \
({                                                      \
    const __m256i __zero = {};                          \
    mpv_8 __mpv = { .vec = (_mpv).vec };                \
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);   \
    (u32_8)_mm512_mask_i64gather_epi32(__zero, __mask,  \
            (__m512i)__mpv.vec, (void *)0, 1);          \
}) /* end of macro */

#define GATHER_u64_8_FROM_MPV8(_mpv)                    \
({                                                      \
    const __m512i __zero = {};                          \
    mpv_8 __mpv = { .vec = (_mpv).vec };                \
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);   \
    (u64_8)_mm512_mask_i64gather_epi64(__zero, __mask,  \
            (__m512i)__mpv.vec, (void *)0, 1);          \
}) /* end of macro */

#define GATHER_u32_8_FROM_STRUCTS(_struct, _field, _mpv)                        \
({                                                                              \
    const __m256i __zero = {};                                                  \
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);   \
    mpv_8 __mpv = { .vec = (_mpv).vec };                                        \
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);                           \
    (u32_8)_mm512_mask_i64gather_epi32(__zero, __mask,                          \
            (__m512i)__mpv.vec, __base, 1);                                     \
}) /* end of macro */

#define GATHER_u64_8_FROM_STRUCTS(_struct, _field, _mpv)                        \
({                                                                              \
    const __m512i __zero = {};                                                  \
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);   \
    mpv_8 __mpv = { .vec = (_mpv).vec };                                        \
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);                           \
    (u64_8)_mm512_mask_i64gather_epi64(__zero, __mask,                          \
            (__m512i)__mpv.vec, __base, 1);                                     \
}) /* end of macro */

#define GATHER_u32_4_FROM_STRUCTS(_struct, _field, _mpv)                        \
({                                                                              \
    const __m128i __zero = {};                                                  \
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);   \
    mpv_4 __mpv = { .vec = (_mpv).vec };                                        \
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);                           \
    (u32_4)_mm256_mmask_i64gather_epi32(__zero, __mask,                         \
            (__m256i)__mpv.vec, __base, 1);                                     \
}) /* end of macro */

#define GATHER_u64_4_FROM_STRUCTS(_struct, _field, _mpv)                        \
({                                                                              \
    const __m256i __zero = {};                                                  \
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);   \
    mpv_4 __mpv = { .vec = (_mpv).vec };                                        \
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);                           \
    (u64_4)_mm256_mmask_i64gather_epi64(__zero, __mask,                         \
            (__m256i)__mpv.vec, __base, 1);                                     \
}) /* end of macro */

/*
 * For each lane where idxvec < idxmax return table[idxvec].  For lanes where
 * idxvec > tsize, return 0;
 */
static inline u32_8 gather_u32_from_lookup_table_x8(const u32_8 idxvec,
        const u32 * const RESTR table,
        const u32 tsize)
{
    const __m256i zero = {};
    const u32 minmax = (tsize < MSB32) ? tsize : MSB32;
    const __mmask8 lanes = VEC_TO_MASK(idxvec < minmax);
    return (u32_8)_mm256_mmask_i32gather_epi32(zero, lanes, (__m256i)idxvec, table, sizeof(u32));
}

/*
 * For each lane where idxvec < tsize return table[idxvec].  For lanes where
 * idxvec > tsize, return 0;
 */
static inline u32_16 gather_u32_from_lookup_table_x16(const u32_16 idxvec,
        const u32 * const RESTR table,
        const u32 tsize)
{
    const __m512i zero = {};
    const u32 minmax = (tsize < MSB32) ? tsize : MSB32;
    const __mmask16 lanes = VEC_TO_MASK(idxvec < minmax);
    return (u32_16)_mm512_mask_i32gather_epi32(zero, lanes, (__m512i)idxvec, table, sizeof(u32));
}

/*
 * For each lane where idxvec < tsize return table[idxvec].  For lanes where
 * idxvec > tsize, return 0;
 */
static inline u64_8 gather_u64_from_lookup_table_x8(const u32_8 idxvec,
        const u64 * const RESTR table,
        const u32 tsize)
{
    const __m512i zero = {};
    const u32 minmax = (tsize < MSB32) ? tsize : MSB32;
    const __mmask8 lanes = VEC_TO_MASK(idxvec < minmax);
    return (u64_8)_mm512_mask_i32gather_epi64(zero, lanes, (__m256i)idxvec, table, sizeof(u64));
}

typedef union {
    u8_64   reg[4];
    u8      u8[256];
} simd_byte_translation_table;

/*
 * For each of 64 lanes of u8, select the corresponding entry in a table of 256 u8 values
 * implemented as four zmm registers where bits 5:0 select a lane and bit 6 selects a set of lanes,
 * and bit 7 selects whether to use the low or high pair, with the results OR'd together.  This is
 * equivalent to: for (i=0; i<64; i++) { out[i] = table[in[i]]; } but one such ganged lookup can be
 * accomplished about once every 5 clock cycles.
 */
static inline u8_64 translate_bytes_x64(const u8_64 in,
                                        const simd_byte_translation_table * const RESTR table)
{
    const __mmask64 b7 = _mm512_movepi8_mask((__m512i)in);
    const u8_64 t0 = (u8_64)_mm512_maskz_permutex2var_epi8(_knot_mask64(b7), (__m512i)table->reg[0],
                     (__m512i)in,
                     (__m512i)table->reg[1]);
    const u8_64 t1 = (u8_64)_mm512_maskz_permutex2var_epi8(b7, (__m512i)table->reg[2], (__m512i)in,
                     (__m512i)table->reg[3]);
    return t0 | t1;
}

#endif /* _SG_UTIL_H_ */
