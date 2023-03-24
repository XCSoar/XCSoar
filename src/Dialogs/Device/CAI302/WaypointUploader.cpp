// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointUploader.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/Factory.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "Device/Driver/CAI302/Internal.hpp"

void
CAI302WaypointUploader::Run(OperationEnvironment &env)
{
  Waypoints waypoints;

  env.SetText(_("Loading Waypoints..."));

  ReadWaypointFile(path, waypoints, WaypointFactory(WaypointOrigin::NONE),
                   env);

  if (waypoints.size() > 9999) {
    env.SetErrorMessage(_("Too many waypoints."));
    return;
  }

  env.SetText(_("Uploading Waypoints"));
  env.SetProgressRange(waypoints.size() + 1);
  env.SetProgressPosition(0);

  device.ClearPoints(env);

  if (!device.EnableBulkMode(env)) {
    env.SetErrorMessage(_("Failed to switch baud rate."));
    return;
  }

  unsigned id = 1;
  for (const auto &i : waypoints) {
    env.SetProgressPosition(id);

    if (!device.WriteNavpoint(id++, *i, env)) {
      env.SetErrorMessage(_("Failed to write waypoint."));
      break;
    }
  }
  device.CloseNavpoints(env);
  device.DisableBulkMode(env);
}
