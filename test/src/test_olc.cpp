#include "Math/FastMath.h"
#include "harness_flight.hpp"
#include "harness_airspace.hpp"

int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  verbose=2;  
  
  // default arguments
  target_noise=0.1;
  output_skip = 5;
  start_alt = 400;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_olc(0,OLC_Sprint),"olc sprint",0);
  ok(test_olc(0,OLC_Classic),"olc classic",0);
  ok(test_olc(0,OLC_FAI),"olc fai",0);

  return exit_status();
}

