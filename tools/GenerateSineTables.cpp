#include "Math/Constants.hpp"
#include "Math/FastTrig.hpp"
#include "Computer/ThermalLocator.hpp"

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
  return DEG_TO_RAD * ((double)x * 360) / INT_ANGLE_RANGE;
}

int
main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  printf("const double SINETABLE[%u] = {", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %.20e,\n", sin(INT_TO_DEG(i)));
  puts("};");

  printf("const short ISINETABLE[%u] = {", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %d,\n", (int)lround(sin(INT_TO_DEG(i)) * 1024));
  puts("};");

  printf("const double INVCOSINETABLE[%u] = {", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++) {
    double x = cos(INT_TO_DEG(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  %.20e,\n", 1.0 / x);
  }
  puts("};");

  printf("#define THERMALRECENCY_SIZE %d\n", ThermalLocator::TLOCATOR_NMAX);
  printf("const double THERMALRECENCY[%d] = {", ThermalLocator::TLOCATOR_NMAX);
  for (unsigned i = 0; i < ThermalLocator::TLOCATOR_NMAX; i++)
    printf("  %.20e,\n", thermal_fn(i));
  puts("};");

  return 0;
}
