#ifndef XCSOAR_MATH_FASTMATH_H
#define XCSOAR_MATH_FASTMATH_H

#include <math.h>

//2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor
//16.16 fixed point representation,

// =================================================================================
// Real2Int
// =================================================================================
static inline int Real2Int(double val)
{
#if (WINDOWS_PC>0)
  val += 68719476736.0*1.5;
  return *((long*)&val) >> 16;
#else
  return (int)val;
#endif
}

static inline int iround(double i) {
    return Real2Int(floor(i+0.5));
}

/*
static inline long lround(double i) {
    return (long)(floor(i+0.5));
}
*/

extern double COSTABLE[4096];
extern double SINETABLE[4096];
extern double INVCOSINETABLE[4096];
extern int ISINETABLE[4096];
extern int ICOSTABLE[4096];

#ifdef __MINGW32__
#define DEG_TO_INT(x) ((unsigned short)(int)((x)*(65536.0/360.0)))>>4
#else
#define DEG_TO_INT(x) ((unsigned short)((x)*(65536.0/360.0)))>>4
#endif

#define invfastcosine(x) INVCOSINETABLE[DEG_TO_INT(x)]
#define ifastcosine(x) ICOSTABLE[DEG_TO_INT(x)]
#define ifastsine(x) ISINETABLE[DEG_TO_INT(x)]
#define fastcosine(x) COSTABLE[DEG_TO_INT(x)]
#define fastsine(x) SINETABLE[DEG_TO_INT(x)]

#ifdef __cplusplus
extern "C" {
#endif

// Fast trig functions
void InitSineTable(void);

#ifdef __cplusplus
}
#endif

#endif
