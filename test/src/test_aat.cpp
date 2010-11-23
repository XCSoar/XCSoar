#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) 
{
  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  turn_speed=5.0;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

#define NUM_FLIGHT 2

  plan_tests(NUM_FLIGHT*2);

  for (int i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(2,k), test_name("target ",2,k),0);
  }
  for (int i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(0,k), test_name("target ",0,k),0);
  }
  return exit_status();
}
