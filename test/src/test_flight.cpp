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

  plan_tests(NUM_TASKS*NUM_WIND);

  for (int j=0; j<NUM_TASKS; j++) {
    for (int i=0; i<NUM_WIND; i++) {
      ok (test_flight_times(j,i), test_name("flight times",j,i),0);
    }
  }
  return exit_status();
}
