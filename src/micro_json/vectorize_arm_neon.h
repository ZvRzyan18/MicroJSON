#ifndef MJS_NEON
#define MJS_NEON
#include <arm_neon.h>
#include "micro_json/parser.h"

/*
 NOTE : this might be slow for memory bounded
 devices.
*/

MJS_INLINE int Neon_AnyOf_s8(uint8x8_t x) {
 return !!(*((uint64_t*)&x));
}

MJS_INLINE int Neon_AnyOf_s16(uint8x16_t x) {
 return Neon_AnyOf_s8(vget_high_u8(x)) | Neon_AnyOf_s8(vget_low_u8(x));
}


MJS_INLINE uint8x16_t Neon_IsWhitespace_s16(int8x16_t x) {
 const int8x16_t c1 = vdupq_n_s8(0x20);
 const int8x16_t c2 = vdupq_n_s8(0x0A);
 const int8x16_t c3 = vdupq_n_s8(0x0D);
 const int8x16_t c4 = vdupq_n_s8(0x09); 
 
 const uint8x16_t tmp1 = vorrq_u8(vceqq_s8(x, c1), vceqq_s8(x, c2));
 const uint8x16_t tmp2 = vorrq_u8(vceqq_s8(x, c3), vceqq_s8(x, c4));
 return vorrq_u8(tmp1, tmp2);
}


MJS_INLINE int8x16_t Neon_IsDigit_s16(int8x16_t x) {
 const int8x16_t c1 = vdupq_n_s8('0');
 const int8x16_t c2 = vdupq_n_s8('9');
 return vandq_s8(vcgeq_s8(x, c1), vcleq_s8(x, c2));
}


MJS_INLINE int Neon_CountNonZero(uint8x16_t v) {
 uint8x16_t tmp1 = vtstq_s8(v, v);
 uint16x8_t tmp2 = vpaddlq_u8(tmp1);
 uint32x4_t tmp3 = vpaddlq_u16(tmp2);
 uint64x2_t tmp4 = vpaddlq_u32(tmp3);
 uint64_t mx = vgetq_lane_u64(tmp4, 0) + vgetq_lane_u64(tmp4, 1);
 /* divide by 255*/
 return (int)((mx * 0x80808081) >> 39);
}


MJS_INLINE uint8_t Neon_PackToBits(uint8x8_t v) {
 uint8x8_t tmp1 = vtst_u8(v, v);
 uint8x8_t bits = vshr_n_u8(tmp1, 7);
 return (vget_lane_u8(bits, 0) << 0) | (vget_lane_u8(bits,1) << 1) | (vget_lane_u8(bits,2) << 2) | (vget_lane_u8(bits,3) << 3) | (vget_lane_u8(bits,4) << 4) | (vget_lane_u8(bits,5) << 5) | (vget_lane_u8(bits,6) << 6) | (vget_lane_u8(bits,7) << 7);
}


MJS_INLINE int Neon_FirstNonZeroIndex(uint8x16_t v) {
 uint16_t lo = (uint16_t)Neon_PackToBits(vget_low_u8(v));
 uint16_t hi = (uint16_t)Neon_PackToBits(vget_high_u8(v));
 uint16_t out = lo | (hi << 8);
 return out ? (MJS_CountTrailingZeroes(out) + 1) : 0;
}


#endif

