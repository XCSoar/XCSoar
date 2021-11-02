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
  printf("constinit const std::array<double, %u> SINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %.20e,\n", sin(IntAngleToRadians(i)));
  puts("};");

  printf("constinit const std::array<short, %u> ISINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++)
    printf("  %d,\n", (int)lround(sin(IntAngleToRadians(i)) * 1024));
  puts("};");

  printf("constinit const std::array<double, %u> INVCOSINETABLE{\n", INT_ANGLE_RANGE);
  for (unsigned i = 0; i < INT_ANGLE_RANGE; i++) {
    double x = cos(IntAngleToRadians(i));
    if ((x >= 0) && (x < 1.0e-8))
      x = 1.0e-8;

    if ((x < 0) && (x > -1.0e-8))
      x = -1.0e-8;

    printf("  %.20e,\n", 1.0 / x);
  }
  puts("};");

  printf("constinit const std::array<double, %d> THERMALRECENCY{\n", THERMALRECENCY_SIZE);
  for (unsigned i = 0; i < THERMALRECENCY_SIZE; i++)
    printf("  %.20e,\n", thermal_fn(i));
  puts("};");

  return 0;
}
