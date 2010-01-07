#include "Math/FastMath.h"
#include "harness_flight.hpp"

int main(int argc, char** argv) 
{
  ::InitSineTable();

  // default arguments
  target_noise=0.1;
  output_skip = 5;

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_airspace(20),"airspace 20",0);
  ok(test_airspace(100),"airspace 100",0);
  
  Airspaces airspaces;
  setup_airspaces(airspaces, 20);
  ok(test_airspace_extra(airspaces),"airspace extra",0);

  return exit_status();
}
