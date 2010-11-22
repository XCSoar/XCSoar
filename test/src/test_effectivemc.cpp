#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) 
{
  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

#define NUM_RUNS 3

  plan_tests(6*NUM_RUNS);

  // tests whether effective mc is calculated correctly
  for (int i=0; i<NUM_RUNS; i++) {
    unsigned j = rand()%NUM_WIND;
    test_effective_mc(3,j);
  }

  return exit_status();
}
