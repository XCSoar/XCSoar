#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  verbose=1;  
  
  // default arguments
  bearing_noise=0;
  target_noise=0.1;
  turn_speed=5.0;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(6);

  terrain_height = 500;
  ok(test_flight(3,0,1.0,true),"high terrain",0);

  terrain_height = 1;
  ok(test_flight(3,0,1.0,true),"basic flight test",0);
  ok(test_abort(0),"abort",0);
  ok(test_goto(0,5),"goto",0);
  ok(test_null(),"null",0);
  ok(test_flight(2,0,1.0,true),"basic flight test",0);

  return exit_status();
}

