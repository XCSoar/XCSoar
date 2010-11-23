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

  unsigned i = rand()%NUM_WIND;
  plan_tests(6);

  // tests whether cruise efficiency is calculated correctly
  test_cruise_efficiency(3,i);

  return exit_status();
}
