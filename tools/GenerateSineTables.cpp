#ifndef FIXED_MATH
#define FIXED_MATH
#endif

#include "Math/Constants.h"
#include "Math/FastMath.h"
#include "Math/fixed.hpp"
#include "ThermalLocator.hpp"

#include <math.h>
#include <stdio.h>

static inline double
thermal_fn(int x)
{
  return exp((-0.2/ThermalLocator::TLOCATOR_NMAX)*pow((double)x, 1.5));
}

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

#if defined(HAVE_MSVCRT) && !defined(__CYGWIN__)
    // Due to non-standard behavior of the microsoft implementation
    // this hack is needed to compile on windows machines
    printf("  %I64dLL,\n",
#else
    printf("  %lldLL,\n",
#endif
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

  printf("#define THERMALRECENCY_SIZE %d\n", ThermalLocator::TLOCATOR_NMAX);
  puts("#ifdef FIXED_MATH");
  printf("const unsigned THERMALRECENCY[] = {\n");
  for (unsigned i = 0; i < ThermalLocator::TLOCATOR_NMAX; i++)
    printf("  %u,\n", (unsigned)(thermal_fn(i) * (double)fixed::resolution));
  puts("#else");
  printf("const fixed THERMALRECENCY[%d] = {", ThermalLocator::TLOCATOR_NMAX);
  for (unsigned i = 0; i < ThermalLocator::TLOCATOR_NMAX; i++)
    printf("  fixed(%.20e),\n", thermal_fn(i));
  puts("#endif");
  puts("};");

  return 0;
}
