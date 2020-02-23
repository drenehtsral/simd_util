
#ifndef _SIMD_TYPES_H_
#define _SIMD_TYPES_H_

#define _SIMD_TYPE_BY_LANE(_lane_type, _lane_count) _lane_type \
    __attribute__((__vector_size__(sizeof(_lane_type) * (_lane_count))))

#define _SIMD_TYPE_BY_SIZE(_lane_type, _total_size) _lane_type \
    __attribute__((__vector_size__(_total_size)))

#define ____STATIC_ASSERT(__line, __cond...) extern int STATIC_ASSERT_ON_LINE ## __line [ 0 - !(__cond) ]
#define __STATIC_ASSERT(_line, _cond) ____STATIC_ASSERT(_line, _cond)
#define STATIC_ASSERT(cond...) __STATIC_ASSERT(__LINE__, (cond))

typedef unsigned char					u8;
typedef unsigned short int				u16;
typedef unsigned int					u32;
typedef unsigned long int				u64;

typedef signed char					i8;
typedef signed short int				i16;
typedef signed int					i32;
typedef signed long int					i64;

typedef float						f32;
typedef double						f64;

#define MSB32	(0x80000000U)
#define MSB64	(0x8000000000000000LU)

STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(f32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);
STATIC_ASSERT(sizeof(f64) == 8);

typedef _SIMD_TYPE_BY_LANE(u8, 16)             		u8_16;
typedef _SIMD_TYPE_BY_LANE(u8, 32)             		u8_32;
typedef _SIMD_TYPE_BY_LANE(u8, 64)             		u8_64;

typedef _SIMD_TYPE_BY_LANE(u16, 8)         u16_8;
typedef _SIMD_TYPE_BY_LANE(u16, 16)        u16_16;
typedef _SIMD_TYPE_BY_LANE(u16, 32)        u16_32;

typedef _SIMD_TYPE_BY_LANE(u32, 4)               u32_4;
typedef _SIMD_TYPE_BY_LANE(u32, 8)               u32_8;
typedef _SIMD_TYPE_BY_LANE(u32, 16)              u32_16;

typedef _SIMD_TYPE_BY_LANE(u64, 2)     u64_2;
typedef _SIMD_TYPE_BY_LANE(u64, 4)     u64_4;
typedef _SIMD_TYPE_BY_LANE(u64, 8)     u64_8;

typedef _SIMD_TYPE_BY_LANE(i8, 16)               i8_16;
typedef _SIMD_TYPE_BY_LANE(i8, 32)               i8_32;
typedef _SIMD_TYPE_BY_LANE(i8, 64)               i8_64;

typedef _SIMD_TYPE_BY_LANE(i16, 8)           i16_8;
typedef _SIMD_TYPE_BY_LANE(i16, 16)          i16_16;
typedef _SIMD_TYPE_BY_LANE(i16, 32)          i16_32;

typedef _SIMD_TYPE_BY_LANE(i32, 4)                 i32_4;
typedef _SIMD_TYPE_BY_LANE(i32, 8)                 i32_8;
typedef _SIMD_TYPE_BY_LANE(i32, 16)                i32_16;

typedef _SIMD_TYPE_BY_LANE(i64, 2)       		i64_2;
typedef _SIMD_TYPE_BY_LANE(i64, 4)       		i64_4;
typedef _SIMD_TYPE_BY_LANE(i64, 8)       		i64_8;

typedef _SIMD_TYPE_BY_LANE(f32, 4)                     f32_4;
typedef _SIMD_TYPE_BY_LANE(f32, 8)                     f32_8;
typedef _SIMD_TYPE_BY_LANE(f32, 16)                    f32_16;

typedef _SIMD_TYPE_BY_LANE(f64, 2)                     f64_2;
typedef _SIMD_TYPE_BY_LANE(f64, 4)                     f64_4;
typedef _SIMD_TYPE_BY_LANE(f64, 8)                     f64_8;


#endif /* _SIMD_TYPES_H_ */
