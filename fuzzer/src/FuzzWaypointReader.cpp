// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint/WaypointReaderSeeYou.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/Waypoints.hpp"
#include "io/MemoryReader.hxx"
#include "io/BufferedReader.hxx"
#include "system/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <tchar.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  Waypoints way_points;

  try {
    MemoryReader mr{{(const std::byte *)data, size}};
    BufferedReader br(mr);
    ParseSeeYou(WaypointFactory{WaypointOrigin::NONE}, way_points, br);
  } catch (...) {
    return EXIT_FAILURE;
  }

  way_points.Optimise();

  return EXIT_SUCCESS;
}
