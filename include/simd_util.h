
#include <immintrin.h>

#include "simd_types.h"

#define TSC_PRECISE() ({unsigned tmp=0; (unsigned long long)__builtin_ia32_rdtscp(&tmp); })
#define TSC_SLOPPY()  ((unsigned long long)__builtin_ia32_rdtsc())

#define OPT_SIZE __attribute__((__optimize__("Os")))

#define VEC_LANES(_inst) (sizeof(_inst) / sizeof((_inst)[0]))
#define VEC_TYPE_LANES(_typ) ({ typeof(_typ) _tmp; VEC_LANES(_tmp); })

#define alignof(_x) __alignof__((_x))
#define offsetof(_t, _f) __builtin_offsetof(_t, _f)

#define RESTR __restrict__

#define CONST_FUNC __attribute__((__const__))
#define PURE_FUNC __attribute__((__pure__))

#include "base_util.h"
#include "mask_util.h"
#include "sg_util.h"

