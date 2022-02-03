
#ifndef _SIMD_TYPES_H_
#define _SIMD_TYPES_H_

#define _SIMD_TYPE_BY_LANE(_lane_type, _lane_count) _lane_type \
    __attribute__((__vector_size__(sizeof(_lane_type) * (_lane_count))))

#define _SIMD_TYPE_BY_SIZE(_lane_type, _total_size) _lane_type \
    __attribute__((__vector_size__(_total_size)))

#define ____STATIC_ASSERT(__line, __cond...) extern int STATIC_ASSERT_ON_LINE ## __line [ 0 - !(__cond) ]
#define __STATIC_ASSERT(_line, _cond) ____STATIC_ASSERT(_line, _cond)
#define STATIC_ASSERT(cond...) __STATIC_ASSERT(__LINE__, (cond))

#define PACKED __attribute__((__packed__))

typedef unsigned char                   u8;
typedef unsigned short int              u16;
typedef unsigned int                    u32;
typedef unsigned long int               u64;

typedef signed char                     i8;
typedef signed short int                i16;
typedef signed int                      i32;
typedef signed long int                 i64;

typedef float                           f32;
typedef double                          f64;

#define MSB32   (0x80000000U)
#define MSB64   (0x8000000000000000LU)

STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(f32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);
STATIC_ASSERT(sizeof(f64) == 8);

typedef _SIMD_TYPE_BY_LANE(u8, 16)      u8_16;
typedef _SIMD_TYPE_BY_LANE(u8, 32)      u8_32;
typedef _SIMD_TYPE_BY_LANE(u8, 64)      u8_64;

typedef _SIMD_TYPE_BY_LANE(u16, 8)      u16_8;
typedef _SIMD_TYPE_BY_LANE(u16, 16)     u16_16;
typedef _SIMD_TYPE_BY_LANE(u16, 32)     u16_32;

typedef _SIMD_TYPE_BY_LANE(u32, 4)      u32_4;
typedef _SIMD_TYPE_BY_LANE(u32, 8)      u32_8;
typedef _SIMD_TYPE_BY_LANE(u32, 16)     u32_16;

typedef _SIMD_TYPE_BY_LANE(u64, 2)      u64_2;
typedef _SIMD_TYPE_BY_LANE(u64, 4)      u64_4;
typedef _SIMD_TYPE_BY_LANE(u64, 8)      u64_8;

typedef _SIMD_TYPE_BY_LANE(i8, 16)      i8_16;
typedef _SIMD_TYPE_BY_LANE(i8, 32)      i8_32;
typedef _SIMD_TYPE_BY_LANE(i8, 64)      i8_64;

typedef _SIMD_TYPE_BY_LANE(i16, 8)      i16_8;
typedef _SIMD_TYPE_BY_LANE(i16, 16)     i16_16;
typedef _SIMD_TYPE_BY_LANE(i16, 32)     i16_32;

typedef _SIMD_TYPE_BY_LANE(i32, 4)      i32_4;
typedef _SIMD_TYPE_BY_LANE(i32, 8)      i32_8;
typedef _SIMD_TYPE_BY_LANE(i32, 16)     i32_16;

typedef _SIMD_TYPE_BY_LANE(i64, 2)      i64_2;
typedef _SIMD_TYPE_BY_LANE(i64, 4)      i64_4;
typedef _SIMD_TYPE_BY_LANE(i64, 8)      i64_8;

typedef _SIMD_TYPE_BY_LANE(f32, 4)      f32_4;
typedef _SIMD_TYPE_BY_LANE(f32, 8)      f32_8;
typedef _SIMD_TYPE_BY_LANE(f32, 16)     f32_16;

typedef _SIMD_TYPE_BY_LANE(f64, 2)      f64_2;
typedef _SIMD_TYPE_BY_LANE(f64, 4)      f64_4;
typedef _SIMD_TYPE_BY_LANE(f64, 8)      f64_8;

/*
 * gcc 9.2.0 seems to do this operation in an ass-backwards way (using five single-cycle
 * instructions where one would do) so it may be worth replacing this with one of those
 * god-awful page-full-of-macro-to-generate-one-instruction constructs...
 */
#define CONV_VECTOR(_v, _t) __builtin_convertvector((_v), _t)

/*
 * On the other hand, the __buitin_has_attribute() construct is way cool!
 */
#define IS_CONST(_e)        __builtin_has_attribute((_e), const)
#define IS_VEC_LEN(_e, _l)  __builtin_has_attribute((_e), vector_size(_l))

#define IS_LANE_SIZE(_e, _l) (sizeof((_e)[0]) == (_l))

/*
 * Various common uses of __builtin_types_compatible_p()
 */
#define EXPR_MATCHES_TYPE(_e, _t)  __builtin_types_compatible_p(typeof(_e), _t)
#define EXPR_TYPES_MATCH(_e0, _e1) __builtin_types_compatible_p(typeof(_e0), typeof(_e1))

/*
 * As silly as it looks to have a macro for this it *really* helps readability for things
 * like lookups in bit vectors, etc.
 */
#define BIT_SIZE(_e) (sizeof(_e) * 8)

typedef union {
    u8_16           u8_16;
    u8_32           u8_32;
    u8_64           u8_64;

    u16_8           u16_8;
    u16_16          u16_16;
    u16_32          u16_32;

    u32_4           u32_4;
    u32_8           u32_8;
    u32_16          u32_16;

    u64_2           u64_2;
    u64_4           u64_4;
    u64_8           u64_8;

    i8_16           i8_16;
    i8_32           i8_32;
    i8_64           i8_64;

    i16_8           i16_8;
    i16_16          i16_16;
    i16_32          i16_32;

    i32_4           i32_4;
    i32_8           i32_8;
    i32_16          i32_16;

    i64_2           i64_2;
    i64_4           i64_4;
    i64_8           i64_8;

    f32_4           f32_4;
    f32_8           f32_8;
    f32_16          f32_16;

    f64_2           f64_2;
    f64_4           f64_4;
    f64_8           f64_8;

    __m512i         __m512i[0];
    __m256i         __m256i[0];
    __m128i         __m128i[0];
    u8              u8 [0];
    u16             u16[0];
    u32             u32[0];
    u64             u64[0];
    i8              i8 [0];
    i16             i16[0];
    i32             i32[0];
    i64             i64[0];
    f32             f32[0];
    f64             f64[0];
} MEGA_UNION;

#define MEGA_UNION_WITH(_t...)      \
union {                             \
    u8_16           u8_16;          \
    u8_32           u8_32;          \
    u8_64           u8_64;          \
                                    \
    u16_8           u16_8;          \
    u16_16          u16_16;         \
    u16_32          u16_32;         \
                                    \
    u32_4           u32_4;          \
    u32_8           u32_8;          \
    u32_16          u32_16;         \
                                    \
    u64_2           u64_2;          \
    u64_4           u64_4;          \
    u64_8           u64_8;          \
                                    \
    i8_16           i8_16;          \
    i8_32           i8_32;          \
    i8_64           i8_64;          \
                                    \
    i16_8           i16_8;          \
    i16_16          i16_16;         \
    i16_32          i16_32;         \
                                    \
    i32_4           i32_4;          \
    i32_8           i32_8;          \
    i32_16          i32_16;         \
                                    \
    i64_2           i64_2;          \
    i64_4           i64_4;          \
    i64_8           i64_8;          \
                                    \
    f32_4           f32_4;          \
    f32_8           f32_8;          \
    f32_16          f32_16;         \
                                    \
    f64_2           f64_2;          \
    f64_4           f64_4;          \
    f64_8           f64_8;          \
                                    \
    __m512i         __m512i[0];     \
    __m256i         __m256i[0];     \
    __m128i         __m128i[0];     \
    u8              u8 [0];         \
    u16             u16[0];         \
    u32             u32[0];         \
    u64             u64[0];         \
    i8              i8 [0];         \
    i16             i16[0];         \
    i32             i32[0];         \
    i64             i64[0];         \
    f32             f32[0];         \
    f64             f64[0];         \
    /* User component */            \
    _t                    ;         \
} /*end of macro */

#endif /* _SIMD_TYPES_H_ */
