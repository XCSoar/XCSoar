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

  plan_tests(4);

  ok(test_abort(0),"abort",0);
  ok(test_goto(0,5,false),"goto",0);
  ok(test_goto(0,5,true),"goto with auto mc",0);
  ok(test_null(),"null",0);

  return exit_status();
}
