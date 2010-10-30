#ifndef FIXED_MATH
#define FIXED_MATH
#endif

#include "Math/Constants.h"
#include "Math/FastMath.h"
#include "Math/fixed.hpp"

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

  puts("#ifdef FIXED_MATH");
  puts("const int SINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n",
           (int)(sin(INT_TO_DEG(i)) * (double)fixed::resolution));
  puts("#else");
  puts("const fixed SINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  fixed(%.20e),\n", sin(INT_TO_DEG(i)));
  puts("#endif");
  puts("};");

  puts("#ifdef FIXED_MATH");
  puts("const int COSTABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n",
           (int)(cos(INT_TO_DEG(i)) * (double)fixed::resolution));
  puts("#else");
  puts("const fixed COSTABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  fixed(%.20e),\n", cos(INT_TO_DEG(i)));
  puts("#endif");
  puts("};");

  puts("const short ISINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n", (int)lround(sin(INT_TO_DEG(i)) * 1024));
  puts("};");

  puts("const short ICOSTABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++)
    printf("  %d,\n", (int)lround(cos(INT_TO_DEG(i)) * 1024));
  puts("};");

  puts("#ifdef FIXED_MATH");
  puts("const fixed::value_t INVCOSINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++) {
    double x = cos(INT_TO_DEG(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  %lldLL,\n",
           (long long)((double)fixed::resolution / x));
  }
  puts("#else");
  puts("const fixed INVCOSINETABLE[4096] = {");
  for (unsigned i = 0; i < 4096; i++) {
    double x = cos(INT_TO_DEG(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  fixed(%.20e),\n", 1.0 / x);
  }
  puts("#endif");
  puts("};");

  return 0;
}
