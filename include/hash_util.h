#ifndef _HASH_UTIL_H_
#define _HASH_UTIL_H_

static inline PURE_FUNC u32 murmur3_u32(const void * const RESTR key, const unsigned len,
                                        const u32 seed)
{
    CONST_FUNC u32 rotl_u32(const u32 x, const u32 r)
    {
        return (x << r) | (x >> (32 - r));
    }
    const u32 c1 = 0xcc9e2d51U;
    const u32 c2 = 0x1b873593U;
    const u32 c3 = 0xe6546b64U;
    const u32 f1 = 0x85ebca6bU;
    const u32 f2 = 0xc2b2ae35U;

    const unsigned nblk = len >> 2;
    const unsigned rem = len & 3;

    const u32 * const RESTR kblk = key;

    unsigned i;

    u32 accum = seed;

    for (i = 0; i < nblk; i++) {
        const u32 t0 = kblk[i];
        const u32 t1 = t0 * c1;
        const u32 t2 = rotl_u32(t1, 15);
        const u32 t3 = t2 * c2;
        const u32 t4 = accum ^ t3;
        const u32 t5 = rotl_u32(t4, 13);
        const u32 t6 = (t5 * 5);
        const u32 t7 = t6 + c3;
        accum = t7;
    }

    if (rem) {
        const u8 * const RESTR kt = (const u8 *)(kblk + nblk);
        u32 tmp = 0;

        for (i = 0; i < rem; i++) {
            tmp |= ((u32)kt[i]) << (i * 8);
        }

        const u32 t0 = tmp;
        const u32 t1 = t0 * c1;
        const u32 t2 = rotl_u32(t1, 15);
        const u32 t3 = t2 * c2;
        accum ^= t3;
    }

    accum ^= len;
    const u32 m0 = accum ^ (accum >> 16);
    const u32 m1 = m0 * f1;
    const u32 m2 = m1 ^ (m1 >> 13);
    const u32 m3 = m2 * f2;
    const u32 m4 = m3 ^ (m3 >> 16);
    return m4;
}

static inline PURE_FUNC u32_8 murmur3_u32_8(const mpv_8 * const RESTR key, const u32_8 len,
        const u32_8 seed)
{
    CONST_FUNC u32_8 rotl_u32_imm(const u32_8 x, const unsigned r)
    {
        return (x << r) | (x >> (32 - r));
    }
    const u32 c1 = 0xcc9e2d51U;
    const u32 c2 = 0x1b873593U;
    const u32 c3 = 0xe6546b64U;
    const u32 f1 = 0x85ebca6bU;
    const u32 f2 = 0xc2b2ae35U;

    const u32_8 nblk = len >> 2;
    const u32_8 rem = len & 3;

    const __mmask8 initial_lanes = VEC_TO_MASK(key->vec > 0);
    __mmask8 lanes = initial_lanes;

    unsigned i, j;
    u32_8 accum = seed;

    for (i = 0; (lanes = (VEC_TO_MASK(i < nblk) & lanes)); i++) {
        const u64_8 ptmp = (u64_8)key->vec + (i * 4);
        const u32_8 t0 = (u32_8)_mm512_mask_i64gather_epi32((__m256i)accum, lanes, (__m512i)ptmp, NULL, 1);
        const u32_8 t1 = t0 * c1;
        const u32_8 t2 = rotl_u32_imm(t1, 15);
        const u32_8 t3 = t2 * c2;
        const u32_8 t4 = accum ^ t3;
        const u32_8 t5 = rotl_u32_imm(t4, 13);
        const u32_8 t6 = (t5 * 5);
        const u32_8 t7 = t6 + c3;
        accum = MUX_ON_MASK(lanes, t7, accum);
    }

    lanes = initial_lanes & VEC_TO_MASK(rem > 0);

    if (lanes) {
        const u32_8 z = {};
        u32_8 tmp = {};

        for (j = 0; j < 8; j++) {
            if ((lanes >> j) & 1) {
                const u8 * const RESTR kt = ((const u8 *)key->mp[j].cp) + (nblk[j] * 4);
                const u8 z = 0;

                for (i = 0; i < 4; i++) {
                    const u8 * tp = (i < rem[j]) ? kt + i : &z;
                    tmp[j] |= ((u32) * tp) << (i * 8);
                }
            }
        }

        const u32_8 t0 = tmp;
        const u32_8 t1 = t0 * c1;
        const u32_8 t2 = rotl_u32_imm(t1, 15);
        const u32_8 t3 = t2 * c2;
        accum ^= MUX_ON_MASK(lanes, t3, z);
    }

    accum ^= len;
    const u32_8 m0 = accum ^ (accum >> 16);
    const u32_8 m1 = m0 * f1;
    const u32_8 m2 = m1 ^ (m1 >> 13);
    const u32_8 m3 = m2 * f2;
    const u32_8 m4 = m3 ^ (m3 >> 16);
    return m4;
}

static inline PURE_FUNC u32_8 murmur3_u32_8_notail(const mpv_8 * const RESTR key, const u32_8 nblk,
        const u32_8 seed)
{
    CONST_FUNC u32_8 rotl_u32_imm(const u32_8 x, const unsigned r)
    {
        return (x << r) | (x >> (32 - r));
    }
    const u32 c1 = 0xcc9e2d51U;
    const u32 c2 = 0x1b873593U;
    const u32 c3 = 0xe6546b64U;
    const u32 f1 = 0x85ebca6bU;
    const u32 f2 = 0xc2b2ae35U;

    const __mmask8 initial_lanes = VEC_TO_MASK(key->vec > 0);
    __mmask8 lanes = initial_lanes;

    unsigned i;
    u32_8 accum = seed;

    for (i = 0; (lanes = (VEC_TO_MASK(i < nblk) & lanes)); i++) {
        const u64_8 ptmp = (u64_8)key->vec + (i * 4);
        const u32_8 t0 = (u32_8)_mm512_mask_i64gather_epi32((__m256i)accum, lanes, (__m512i)ptmp, NULL, 1);
        const u32_8 t1 = t0 * c1;
        const u32_8 t2 = rotl_u32_imm(t1, 15);
        const u32_8 t3 = t2 * c2;
        const u32_8 t4 = accum ^ t3;
        const u32_8 t5 = rotl_u32_imm(t4, 13);
        const u32_8 t6 = (t5 * 5);
        const u32_8 t7 = t6 + c3;
        accum = MUX_ON_MASK(lanes, t7, accum);
    }

    accum ^= nblk * 4;
    const u32_8 m0 = accum ^ (accum >> 16);
    const u32_8 m1 = m0 * f1;
    const u32_8 m2 = m1 ^ (m1 >> 13);
    const u32_8 m3 = m2 * f2;
    const u32_8 m4 = m3 ^ (m3 >> 16);
    return m4;
}

//static const u8 _rss_iv[48] = {
//    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
//    0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
//    0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
//    0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
//    0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa
//};
//
/////*
// * TODO:  Need a non-slow version and a unit test.
// */
//static PURE_FUNC u32 slow_rss(const u32 * const RESTR in, const unsigned n,
//        const void * const RESTR rss_iv)
//{
//    const u32 * iv = (u32 *)rss_iv;
//    u32 accum = 0;
//    unsigned i,j;
//    for (i = 0; i < n; i++) {
//        const u32 x = htonl(in[i]);
//        for (j = 0; j < 32; j++) {
//            const u32 mask = 0U - ((x >> (31 - j)) & 1);
//            const u64 window = (((u64)htonl(iv[i]) << j) | ((u64)htonl(iv[i+1]) >> (32-j))) & 0xFFFFFFFFU;
//            accum ^= mask & window;
//        }
//    }
//    return accum;
//}

#endif /* _HASH_UTIL_H_ */
