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

  plan_tests(6*NUM_WIND);

  // tests whether effective mc is calculated correctly
  for (int i=0; i<NUM_WIND; i++) {
    test_effective_mc(3,i);
  }

  return exit_status();
}
