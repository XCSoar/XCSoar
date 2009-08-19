#include "Math/FastMath.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.0174532925199432958
#define RAD_TO_DEG 57.2957795131
#endif

double COSTABLE[4096];
double SINETABLE[4096];
double INVCOSINETABLE[4096];
int ISINETABLE[4096];
int ICOSTABLE[4096];

void InitSineTable(void)
{
  int i;
  double angle;
  double cosa, sina;

  for(i=0;i<4096; i++)
    {
      angle = DEG_TO_RAD*((double)i*360)/4096;
      cosa = cos(angle);
      sina = sin(angle);
      SINETABLE[i] = sina;
      COSTABLE[i] = cosa;
      ISINETABLE[i] = iround(sina*1024);
      ICOSTABLE[i] = iround(cosa*1024);
      if ((cosa>0) && (cosa<1.0e-8)) {
        cosa = 1.0e-8;
      }
      if ((cosa<0) && (cosa>-1.0e-8)) {
        cosa = -1.0e-8;
      }
      INVCOSINETABLE[i] = 1.0/cosa;
    }
}
