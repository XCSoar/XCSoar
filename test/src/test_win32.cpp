#include "Math/FastMath.h"
#include "harness_flight.hpp"

#include <windows.h>

int main(int argc, char**argv)
{
  ::InitSineTable();

  bearing_noise=0;
  target_noise=0.1;
  turn_speed=5.0;
  output_skip = 5;

  plan_tests(8);

  ok(test_flight(2,0,1.0,true),"basic flight test",0);

  // airspace
  ok(test_airspace(10),"airspace 10",0);
  ok(test_airspace(100),"airspace 100",0);
  
  Airspaces airspaces;
  setup_airspaces(airspaces, 20);
  ok(test_airspace_extra(airspaces),"airspace extra",0);

  // modes
  ok(test_abort(0),"abort",0);
  ok(test_goto(0,5,false),"goto",0);
  ok(test_goto(0,5,true),"goto with auto mc",0);
  ok(test_null(),"null",0);
  
  return exit_status();
}

// PPC2003
// 212 seconds 
// 15 ms per cycle

// PC
// 11 seconds
// 0.8 ms per cycle

// 13885 cycles
// 20x slower

