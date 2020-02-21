
#include <immintrin.h>

#include "simd_types.h"

#define TSC_PRECISE() ({int tmp=0; (unsigned long long)__rdtscp(&tmp); })
#define TSC_SLOPPY()  ((unsigned long long)__builtin_ia32_rdtsc())

#define VEC_LANES(_inst) (sizeof(_inst) / sizeof((_inst)[0]))
#define VEC_TYPE_LANES(_typ) ({ typeof(_typ) _tmp; VEC_LANES(_tmp); })

#define alignof(_x) __alignof__((_x))

#include "base_util.h"
