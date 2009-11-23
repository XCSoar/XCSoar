#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) 
{
  ::InitSineTable();

  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  turn_speed=5.0;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  int n_wind = 3;

  plan_tests(n_wind);

  // tests whether flying at VOpt for OR task is optimal
  for (int i=0; i<n_wind; i++) {
    ok (test_speed_factor(3,i), "vopt or",0);
  }
  return exit_status();
}
