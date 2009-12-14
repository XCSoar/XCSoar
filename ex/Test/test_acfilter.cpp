#include "Math/FastMath.h"
#include "harness_flight.hpp"

#include "Util/AircraftStateFilter.hpp"
extern AircraftStateFilter *aircraft_filter;

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

  AIRCRAFT_STATE dummy;
  aircraft_filter = new AircraftStateFilter(dummy, 120.0);

  plan_tests(1);
  ok(test_flight(1,0,1.0,true),"basic flight test",0);

  delete aircraft_filter;

  return exit_status();
}

/*
  use get_speed() and get_climb_rate() as inputs to AirspacePerformanceModel()
  as a potential constructor for short range.

  look for intersections along this line (possibly with vert speed margins)

  so, four airspace warning queries:
   -- inside
   -- intersection in short duration (next 10 seconds):  warning
   -- intersection in medium duration (based on filter), 1 minute?
   -- long duration based on task track (display only highlighted)

*/
