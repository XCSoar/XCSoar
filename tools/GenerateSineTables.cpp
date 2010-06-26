#include "Math/Constants.h"
#include "Math/FastMath.h"

#include <math.h>
#include <stdio.h>

static inline double
INT_TO_DEG(int x)
{
  return DEG_TO_RAD * ((double)x * 360) / 4096;
}

int
main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  puts("const fixed SINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  fixed(%.20e),\n", sin(INT_TO_DEG(i)));
  puts("};");

  puts("const fixed COSTABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  fixed(%.20e),\n", cos(INT_TO_DEG(i)));
  puts("};");

  puts("const int ISINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n", iround(sin(INT_TO_DEG(i)) * 1024));
  puts("};");

  puts("const int ICOSTABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n", iround(cos(INT_TO_DEG(i)) * 1024));
  puts("};");

  puts("const fixed INVCOSINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++) {
    double x = cos(INT_TO_DEG(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  fixed(%.20e),\n", 1.0 / x);
  }
  puts("};");

  return 0;
}
