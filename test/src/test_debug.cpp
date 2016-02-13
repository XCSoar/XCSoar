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

#include "harness_wind.hpp"
#include "harness_task.hpp"
#include "harness_flight.hpp"
#include "Contest/Solvers/ContestDijkstra.hpp"
#include "OS/ConvertPathName.hpp"
#include "test_debug.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

int n_samples = 0;
int interactive = 0;
int output_skip = 5;

AutopilotParameters autopilot_parms;

int terrain_height = 1;
AllocatedPath replay_file = Path(_T("test/data/0asljd01.igc"));
AllocatedPath waypoint_file = Path(_T("test/data/waypoints_geo.wpt"));
AllocatedPath task_file = nullptr;
double range_threshold = 15000;

#ifdef INSTRUMENT_ZERO
extern unsigned long zero_skipped;
extern unsigned long zero_total;
#endif

void PrintDistanceCounts() {
  if (n_samples) {
    printf("# Instrumentation\n");
    printf("#    (total cycles %d)\n#\n",n_samples);
#ifdef INSTRUMENT_ZERO
    if (zero_total) {
      printf("#    ZeroFinder total %ld\n",zero_total);
      printf("#    ZeroFinder %%skipped %d\n",(int)(100*zero_skipped/zero_total));
    }
#endif
  }
  n_samples = 0;
#ifdef INSTRUMENT_ZERO
  zero_skipped = 0;
  zero_total = 0;
#endif
}

/** 
 * Wait-for-key prompt
 * 
 * @param time time of simulation
 * 
 * @return character received by keyboard
 */
char
WaitPrompt() {
  if (interactive) {
    puts("# [enter to continue]");
    return getchar();
  }
  return 0;
}

bool
ParseArgs(int argc, char** argv)
{
  // initialise random number generator once per test program
  srand(0);

  while (1)    {
    static struct option long_options[] =
      {
	/* These options set a flag. */
	{"verbose", optional_argument,       0, 'v'},
	{"interactive", optional_argument,   0, 'i'},
	{"startalt", required_argument,   0, 'a'},
	{"bearingnoise", required_argument,   0, 'n'},
	{"outputskip", required_argument,       0, 's'},
	{"targetnoise", required_argument,       0, 't'},
	{"turnspeed", required_argument,       0, 'r'},
	{"igc", required_argument,       0, 'f'},
	{"task", required_argument,       0, 'x'},
	{"waypoints", required_argument,       0, 'w'},
	{"rangethreshold", required_argument,       0, 'd'},
	{0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int c = getopt_long (argc, argv, "s:v:i:n:t:r:a:f:x:w:d:",
                         long_options, &option_index);
    /* Detect the end of the options. */
    if (c == -1)
      break;
    
    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
	break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
	printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'f':
      replay_file = PathName(optarg);
      break;
    case 'w':
      waypoint_file = PathName(optarg);
      break;
    case 'x':
      task_file = PathName(optarg);
      break;
    case 'd':
      range_threshold = atof(optarg);
      break;
    case 'a':
      autopilot_parms.start_alt = atof(optarg);
      break;
    case 's':
      output_skip = atoi(optarg);
      break;
    case 'v':
      if (optarg) {
        verbose = atoi(optarg);
      } else {
        verbose = 1;
      }
      break;
    case 'n':
      autopilot_parms.bearing_noise = atof(optarg);
      break;
    case 't':
      autopilot_parms.target_noise = atof(optarg);
      break;
    case 'r':
      autopilot_parms.turn_speed = atof(optarg);
      break;
    case 'i':
      if (optarg) {
        interactive = atoi(optarg);
      } else {
        interactive = 1;
      }
      break;
    case '?':
      /* getopt_long already printed an error message. */

      for (unsigned i=0; i+1< sizeof(long_options)/sizeof(option); i++) {
        switch (long_options[i].has_arg) {
        case 0:
          printf(" --%s %c\n", long_options[i].name, 
                 long_options[i].val);
          break;
        case 1:
          printf(" --%s -%c value\n", long_options[i].name, 
                 long_options[i].val);
          break;
        case 2:
          printf(" --%s -%c [value]\n", long_options[i].name, 
                 long_options[i].val);
        }
      }
      abort();
      return false;
      break;      
    default:
      return false;
    }
  }

  if (interactive && !verbose) {
    verbose=1;
  }

  return true;
}

const char* GetTestName(const char* in, int task_num, int wind_num)
{
  static char buffer[80];
  sprintf(buffer,"%s (task %s, wind %s)", in, task_name(task_num), wind_name(wind_num));
  return buffer;
}
