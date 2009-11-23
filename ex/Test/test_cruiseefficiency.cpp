#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) 
{
  ::InitSineTable();

  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  int n_wind = 2;

  plan_tests(6*n_wind);

  // tests whether cruise efficiency is calculated correctly
  for (int i=0; i<n_wind; i++) {
    test_cruise_efficiency(3,i);
  }

  return exit_status();
}
