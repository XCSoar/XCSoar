#include "Math/Constants.hpp"
#include "Math/FastTrig.hpp"
#include "Computer/ThermalRecency.hpp"

#include <math.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  printf("#include <array>\n");
  printf("const std::array<double, %u> SINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %.20e,\n", sin(IntAngleToRadians(i)));
  puts("};");

  printf("const std::array<short, %u> ISINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %d,\n", (int)lround(sin(IntAngleToRadians(i)) * 1024));
  puts("};");

  printf("const std::array<double, %u> INVCOSINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++) {
    double x = cos(IntAngleToRadians(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  %.20e,\n", 1.0 / x);
  }
  puts("};");

  printf("#define THERMALRECENCY_SIZE %d\n", ThermalLocator::TLOCATOR_NMAX);
  printf("const std::array<double, %d> THERMALRECENCY{\n", ThermalLocator::TLOCATOR_NMAX);
  for (unsigned i = 0; i < ThermalLocator::TLOCATOR_NMAX; i++)
    printf("  %.20e,\n", thermal_fn(i));
  puts("};");

  return 0;
}
