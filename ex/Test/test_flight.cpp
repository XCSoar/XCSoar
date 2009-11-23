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

  int n_wind = 3;

  plan_tests(5*n_wind);

  for (int i=0; i<n_wind; i++) {
    ok (test_flight(3,i), "flight or",0);
    ok (test_flight(4,i), "flight dash",0);
    ok (test_flight(1,i), "flight fai",0);
    ok (test_flight(2,i), "flight aat",0);
    ok (test_flight(0,i), "flight mixed",0);
  }
  return exit_status();
}
