#include <arm_neon.h>
#include "micro_json/parser.h"

#define MJS_NEON

#if defined(__GNUC__) || defined(__clang__)
#define __FORCE_INLINE__ static __inline__ __attribute__((always_inline, unused, hot))
#elif defined(_MSC_VER)
#define __FORCE_INLINE__ static __forceinline
#else
#define __FORCE_INLINE__ static
#endif

/*
 NOTE : this might be slow for memory bounded
 devices.
*/

__FORCE_INLINE__ int Neon_AnyOf_s8(int8x8_t x) {
 return *((unsigned long*)&x) != 0;
}

__FORCE_INLINE__ int Neon_AnyOf_s16(int8x16_t x) {
 return Neon_AnyOf_s8(vget_high_s8(x)) | Neon_AnyOf_s8(vget_low_s8(x));
}


__FORCE_INLINE__ int8x16_t Neon_IsWhitespace_s16(int8x16_t x) {
 const int8x16_t c1 = vdupq_n_s8('\0');
 const int8x16_t c2 = vdupq_n_s8(0x20);
 const int8x16_t c3 = vdupq_n_s8(0x0A);
 const int8x16_t c4 = vdupq_n_s8(0x0D);
 const int8x16_t c5 = vdupq_n_s8(0x09); 
 
 const int8x16_t tmp1 = vorrq_s8(vceqq_s8(x, c1), vceqq_s8(x, c2));
 const int8x16_t tmp2 = vorrq_s8(vceqq_s8(x, c3), vceqq_s8(x, c4));
 return vorrq_s8(vorrq_s8(tmp1, tmp2), vceqq_s8(x, c5));
}


__FORCE_INLINE__ int8x16_t Neon_IsDigit_s16(int8x16_t x) {
 const int8x16_t c1 = vdupq_n_s8('0');
 const int8x16_t c2 = vdupq_n_s8('9');
 return vandq_s8(vcgeq_s8(x, c1), vcleq_s8(x, c2));
}



__FORCE_INLINE__ uint64x2_t Neon_UpdateCounterIfAny(uint64x2_t counter, int8x8_t cond, int incr) {

#if defined(__aarch64__)

 int8_t any = vmaxv_s8(cond);
 uint64x2_t mask = vdupq_n_u64(-any);
 uint64x2_t one = vdupq_n_u64((uint64_t)incr);
 return vaddq_u64(counter, vandq_u64(mask, one));

#elif defined(__arm__)

 /*
  might be a lot slower
 */
 uint8x8_t abs8 = vabd_s8(cond, cond);
 uint8x8_t max4 = vpmax_u8(abs8, abs8);
 uint8x8_t max2 = vpmax_u8(max4, max4);
 uint8x8_t max1 = vpmax_u8(max2, max2);
 uint8x8_t mask8 = vceq_u8(max1, vdup_n_u8(0));
 
 mask8 = vmvn_u8(mask8);
 uint64x2_t mask64 = vreinterpretq_u64_u8(vcombine_u8(mask8, mask8));
 uint64x2_t incr_v = vdupq_n_u64(incr);
 return vaddq_u64(counter, vandq_u64(mask64, incr_v));
 
#endif
}


__FORCE_INLINE__ int Neon_CountNonZero(int8x16_t v) {
 uint8x16_t tmp1 = vtstq_s8(v, v);
 uint16x8_t tmp2 = vpaddlq_u8(tmp1);
 uint32x4_t tmp3 = vpaddlq_u16(tmp2);
 uint64x2_t tmp4 = vpaddlq_u32(tmp3);
 uint64_t mx = vgetq_lane_u64(tmp4, 0) + vgetq_lane_u64(tmp4, 1);
 /* divide by 255*/
 return (int)((mx * 0x80808081) >> 39);
}

/*
__FORCE_INLINE__ uint8_t Neon_PackToBits(int8x8_t v) {
 uint8x8_t tmp1 = vtst_s8(v, v);
 uint8x8_t bits = vshr_n_u8(tmp1, 7);
 return (vget_lane_u8(bits,0) << 0) | (vget_lane_u8(bits,1) << 1) | (vget_lane_u8(bits,2) << 2) | (vget_lane_u8(bits,3) << 3) | (vget_lane_u8(bits,4) << 4) | (vget_lane_u8(bits,5) << 5) | (vget_lane_u8(bits,6) << 6) | (vget_lane_u8(bits,7) << 7);
}

__FORCE_INLINE__ int Neon_FirstNonZeroIndex(int8x16_t v) {
 uint16_t lo = (uint16_t)Neon_PackToBits(vget_low_s8(v));
 uint16_t hi = (uint16_t)Neon_PackToBits(vget_high_s8(v)) << 8;
 return __builtin_ctz((lo | hi) & 0xFFFF);
}
*/


