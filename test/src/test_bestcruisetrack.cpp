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

  plan_tests(NUM_WIND-1);

  for (int i=1; i<NUM_WIND; i++) {
    test_bestcruisetrack(2,i);
  }
  return exit_status();
}
