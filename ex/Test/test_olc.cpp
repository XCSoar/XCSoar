#include "Math/FastMath.h"
#include "harness_flight.hpp"
#include "harness_airspace.hpp"

int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  verbose=2;  
  
  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  turn_speed=5.0;
  output_skip = 5;
  start_alt = 400;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(1);

  ok(test_flight(1,0,1.0,true),"olc",0);

  return exit_status();
}

