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
        u64	addr : 63,
            inv  : 1;
    };
} masked_ptr_u;

typedef union {
    i64_4		vec;
    masked_ptr_u	mp[4];
} mpv_4;

typedef union {
    i64_8		vec;
    masked_ptr_u	mp[8];
    mpv_4	v4[2];
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

#define GATHER_u32_4_FROM_MPV4(_mpv)						\
({										\
    const __m128i __zero = {};							\
    mpv_4 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);				\
    (u32_4)_mm256_mmask_i64gather_epi32(__zero, __mask,				\
            (__m256i)__mpv.vec, (void *)0, 1);					\
}) /* end of macro */

#define GATHER_u64_4_FROM_MPV4(_mpv)						\
({										\
    const __m256i __zero = {};							\
    mpv_4 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);				\
    (u64_4)_mm256_mmask_i64gather_epi64(__zero, __mask,				\
            (__m256i)__mpv.vec, (void *)0, 1);					\
}) /* end of macro */

#define GATHER_u32_8_FROM_MPV8(_mpv)						\
({										\
    const __m256i __zero = {};							\
    mpv_8 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);				\
    (u32_8)_mm512_mask_i64gather_epi32(__zero, __mask,				\
            (__m512i)__mpv.vec, (void *)0, 1);					\
}) /* end of macro */

#define GATHER_u64_8_FROM_MPV8(_mpv)						\
({										\
    const __m512i __zero = {};							\
    mpv_8 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);				\
    (u64_8)_mm512_mask_i64gather_epi64(__zero, __mask,				\
            (__m512i)__mpv.vec, (void *)0, 1);					\
}) /* end of macro */

#define GATHER_u32_8_FROM_STRUCTS(_struct, _field, _mpv)			\
({										\
    const __m256i __zero = {};							\
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);	\
    mpv_8 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);				\
    (u32_8)_mm512_mask_i64gather_epi32(__zero, __mask,				\
            (__m512i)__mpv.vec, __base, 1);					\
}) /* end of macro */

#define GATHER_u64_8_FROM_STRUCTS(_struct, _field, _mpv)			\
({										\
    const __m512i __zero = {};							\
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);	\
    mpv_8 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_8_fixup_mask(&__mpv);				\
    (u64_8)_mm512_mask_i64gather_epi64(__zero, __mask,				\
            (__m512i)__mpv.vec, __base, 1);					\
}) /* end of macro */

#define GATHER_u32_4_FROM_STRUCTS(_struct, _field, _mpv)			\
({										\
    const __m128i __zero = {};							\
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);	\
    mpv_4 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);				\
    (u32_4)_mm256_mmask_i64gather_epi32(__zero, __mask,				\
            (__m256i)__mpv.vec, __base, 1);					\
}) /* end of macro */

#define GATHER_u64_4_FROM_STRUCTS(_struct, _field, _mpv)			\
({										\
    const __m256i __zero = {};							\
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);	\
    mpv_4 __mpv = { .vec = (_mpv).vec };					\
    const __mmask8 __mask = mpv_4_fixup_mask(&__mpv);				\
    (u64_4)_mm256_mmask_i64gather_epi64(__zero, __mask,				\
            (__m256i)__mpv.vec, __base, 1);					\
}) /* end of macro */



#endif /* _SG_UTIL_H_ */