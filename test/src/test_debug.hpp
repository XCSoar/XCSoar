/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include "Printing.hpp"

#include "Replay/TaskAutoPilot.hpp"
#include "OS/Path.hpp"

extern "C" {
#include "tap.h"
}

#include <iosfwd>
#include <string>

extern int n_samples;
void PrintDistanceCounts();
char WaitPrompt();
extern int interactive;
extern int verbose;
extern int output_skip;

extern AutopilotParameters autopilot_parms;

extern int terrain_height;
const char* GetTestName(const char* in, int task_num, int wind_num);
extern AllocatedPath replay_file;
extern AllocatedPath task_file;
extern AllocatedPath waypoint_file;
extern double range_threshold;

bool ParseArgs(int argc, char** argv);

#endif
