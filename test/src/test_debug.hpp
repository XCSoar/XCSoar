// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Printing.hpp"

#include "Replay/TaskAutoPilot.hpp"
#include "system/Path.hpp"

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
