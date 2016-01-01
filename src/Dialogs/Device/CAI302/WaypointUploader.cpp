/*
Copyright_License {

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

  if (!ReadWaypointFile(path, waypoints, WaypointFactory(WaypointOrigin::NONE),
                        env)) {
    env.SetErrorMessage(_("Failed to load file."));
    return;
  }

  if (waypoints.size() > 9999) {
    env.SetErrorMessage(_("Too many waypoints."));
    return;
  }

  env.SetText(_("Uploading Waypoints"));
  env.SetProgressRange(waypoints.size() + 1);
  env.SetProgressPosition(0);

  if (!device.ClearPoints(env)) {
    if (!env.IsCancelled())
      env.SetErrorMessage(_("Failed to erase waypoints."));
    return;
  }

  if (!device.EnableBulkMode(env)) {
    if (!env.IsCancelled())
      env.SetErrorMessage(_("Failed to switch baud rate."));
    return;
  }

  unsigned id = 1;
  for (const auto &i : waypoints) {
    if (env.IsCancelled())
      break;

    env.SetProgressPosition(id);

    if (!device.WriteNavpoint(id++, *i, env)) {
      if (!env.IsCancelled())
        env.SetErrorMessage(_("Failed to write waypoint."));
      break;
    }
  }

  device.DisableBulkMode(env);
}
