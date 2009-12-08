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

  plan_tests(3);

  ok(test_airspace(10),"airspace 10",0);
  ok(test_airspace(50),"airspace 50",0);
  
  Airspaces airspaces;
  setup_airspaces(airspaces, 20);
  ok(test_airspace_extra(airspaces),"airspace extra",0);

  return exit_status();
}
