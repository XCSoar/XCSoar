/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Math/FastMath.h"
#include "harness_flight.hpp"

#include <windows.h>

int main(int argc, char**argv)
{
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

