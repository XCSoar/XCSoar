/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "harness_airspace.hpp"

int main(int argc, char** argv) {
  
  // default arguments
  autopilot_parms.realistic();
  autopilot_parms.start_alt = fixed(400);

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_olc(0,OLC_Sprint),"olc sprint",0);
  ok(test_olc(0,OLC_Classic),"olc classic",0);
  ok(test_olc(0,OLC_FAI),"olc fai",0);

  return exit_status();
}

