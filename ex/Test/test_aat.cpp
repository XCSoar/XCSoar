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

  plan_tests(NUM_WIND*2);

  for (int i=0; i<NUM_WIND; i++) {
    ok (test_aat(2,i), test_name("target aat",2,i),0);
    ok (test_aat(0,i), test_name("target mixed",0,i),0);
  }
  return exit_status();
}
