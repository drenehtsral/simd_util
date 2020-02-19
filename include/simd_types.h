
#ifndef __SIMD_TYPES_H_
#define __SIMD_TYPES_H_

#define _SIMD_TYPE_BY_LANE(_lane_type, _lane_count) _lane_type \
    __attribute__((__vector_size__(sizeof(_lane_type) * (_lane_count))))

#define _SIMD_TYPE_BY_SIZE(_lane_type, _total_size) _lane_type \
    __attribute__((__vector_size__(_total_size)))


typedef _SIMD_TYPE_BY_LANE(unsigned char, 16)             u8_16;
typedef _SIMD_TYPE_BY_LANE(unsigned char, 32)             u8_32;
typedef _SIMD_TYPE_BY_LANE(unsigned char, 64)             u8_64;

typedef _SIMD_TYPE_BY_LANE(unsigned short int, 8)         u16_8;
typedef _SIMD_TYPE_BY_LANE(unsigned short int, 16)        u16_16;
typedef _SIMD_TYPE_BY_LANE(unsigned short int, 32)        u16_32;

typedef _SIMD_TYPE_BY_LANE(unsigned int, 4)               u32_4;
typedef _SIMD_TYPE_BY_LANE(unsigned int, 8)               u32_8;
typedef _SIMD_TYPE_BY_LANE(unsigned int, 16)              u32_16;

typedef _SIMD_TYPE_BY_LANE(unsigned long long int, 2)     u64_2;
typedef _SIMD_TYPE_BY_LANE(unsigned long long int, 4)     u64_4;
typedef _SIMD_TYPE_BY_LANE(unsigned long long int, 8)     u64_8;

typedef _SIMD_TYPE_BY_LANE(signed char, 16)               i8_16;
typedef _SIMD_TYPE_BY_LANE(signed char, 32)               i8_32;
typedef _SIMD_TYPE_BY_LANE(signed char, 64)               i8_64;

typedef _SIMD_TYPE_BY_LANE(signed short int, 8)           i16_8;
typedef _SIMD_TYPE_BY_LANE(signed short int, 16)          i16_16;
typedef _SIMD_TYPE_BY_LANE(signed short int, 32)          i16_32;

typedef _SIMD_TYPE_BY_LANE(signed int, 4)                 i32_4;
typedef _SIMD_TYPE_BY_LANE(signed int, 8)                 i32_8;
typedef _SIMD_TYPE_BY_LANE(signed int, 16)                i32_16;

typedef _SIMD_TYPE_BY_LANE(signed long long int, 2)       i64_2;
typedef _SIMD_TYPE_BY_LANE(signed long long int, 4)       i64_4;
typedef _SIMD_TYPE_BY_LANE(signed long long int, 8)       i64_8;

typedef _SIMD_TYPE_BY_LANE(float, 4)                      f32_4;
typedef _SIMD_TYPE_BY_LANE(float, 8)                      f32_8;
typedef _SIMD_TYPE_BY_LANE(float, 16)                     f32_16;

typedef _SIMD_TYPE_BY_LANE(double, 2)                     f64_2;
typedef _SIMD_TYPE_BY_LANE(double, 4)                     f64_4;
typedef _SIMD_TYPE_BY_LANE(double, 8)                     f64_8;


#endif /* _SIMD_TYPES_H_ */
