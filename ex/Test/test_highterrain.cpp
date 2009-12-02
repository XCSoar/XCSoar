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

#define NUM_TERRAIN 5

  plan_tests(NUM_TERRAIN);

  for (int j=0; j<NUM_TERRAIN; j++) {
    terrain_height = (int)((800)*(j+1)/(NUM_TERRAIN*1.0));

    unsigned i = rand()%NUM_WIND;
    ok (test_flight_times(7,i), test_name("high terrain",7,i),0);
  }
  return exit_status();
}
