#include <arm_neon.h>
#include "micro_json/parser.h"

#define MJS_NEON

#if defined(__GNUC__) || defined(__clang__)
#define __FORCE_INLINE__ static __attribute__((always_inline, unused))
#else
#define __FORCE_INLINE__ static
#endif



__FORCE_INLINE__ int Neon_AnyOf_s8(int8x8_t x) {
 return x[0] || x[1] || x[2] || x[3] || x[4] || x[5] || x[6] || x[7];
}

__FORCE_INLINE__ int Neon_AnyOf_s16(int8x16_t x) {
 int8x8_t a, b;
 a[0] = x[0]; a[1] = x[1]; a[2] = x[2]; a[3] = x[3];
 a[4] = x[4]; a[5] = x[5]; a[6] = x[6]; a[7] = x[7];

 b[0] = x[8]; b[1] = x[9]; b[2] = x[10]; b[3] = x[11];
 b[4] = x[12]; b[5] = x[13]; b[6] = x[14]; b[7] = x[15];

 return Neon_AnyOf_s8(vorr_s8(a, b));
}


__FORCE_INLINE__ int8x8_t Neon_IsWhitespace_s8(int8x8_t x) {
 const int8x8_t c1 = vdup_n_s8('\0');
 const int8x8_t c2 = vdup_n_s8(0x20);
 const int8x8_t c3 = vdup_n_s8(0x0A);
 const int8x8_t c4 = vdup_n_s8(0x0D);
 const int8x8_t c5 = vdup_n_s8(0x09); 
 
 const int8x8_t tmp1 = vorr_s8(vceq_s8(x, c1), vceq_s8(x, c2));
 const int8x8_t tmp2 = vorr_s8(vceq_s8(x, c3), vceq_s8(x, c4));
 return vorr_s8(vorr_s8(tmp1, tmp2), vceq_s8(x, c5));
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


__FORCE_INLINE__ int8x8_t Neon_IsDigit_s8(int8x8_t x) {
 const int8x8_t c1 = vdup_n_s8('0');
 const int8x8_t c2 = vdup_n_s8('9');
 return vand_s8(vcge_s8(x, c1), vcle_s8(x, c2));
}


__FORCE_INLINE__ int8x16_t Neon_IsDigit_s16(int8x16_t x) {
 const int8x16_t c1 = vdupq_n_s8('0');
 const int8x16_t c2 = vdupq_n_s8('9');
 return vandq_s8(vcgeq_s8(x, c1), vcleq_s8(x, c2));
}



