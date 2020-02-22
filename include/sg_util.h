#ifndef _SG_UTIL_H_
#define _SG_UTIL_H_

// Note to self:
//
// Define a pointer-vector type that explicitly uses the MSB as an "invalid" bit,
// such that scatter_gather operations can do this:
// input const i64_8 ptrs;
// const i64_8 zero = {};
// __mmask8 mask = _mm512_cmpge_epi64(ptrs, zero);
// out = gather(ptrs, mask);

// Since bit 63 (if x64_64 ever grows beyond 48 bits of VA) will always be in
// kernel addess space and thus verboten this is a tidy way to encode validity
// in the members of the pointer vector.

#define GATHER_u32_8_FROM_STRUCTS(_struct, _field, _mask, _ptr_vec_8)		\
({										\
    const void * __base = (const void *) __builtin_offsetof(_struct, _field);	\
    const __mmask8 __mask = (__mmask8) (_mask);					\
    const i64_8 __ptrs = (i64_8)(_ptr_vec_8);					\
    const __m256i __zero = {};							\
    (u32_8)_mm512_mask_i64gather_epi32(__zero, __mask, (__m512i)__ptrs, __base, 1);	\
}) /* end of macro */

#endif /* _SG_UTIL_H_ */