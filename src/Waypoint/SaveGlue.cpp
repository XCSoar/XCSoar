// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointGlue.hpp"
#include "CupWriter.hpp"
#include "LogFile.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "LocalPath.hpp"

void
WaypointGlue::SaveWaypoints(const Waypoints &way_points)
{
  const auto path = LocalPath(_T("user.cup"));

  FileOutputStream file(path);
  BufferedOutputStream writer(file);

  WriteCup(writer, way_points, WaypointOrigin::USER);

  writer.Flush();
  file.Commit();

  LogFormat(_T("Waypoint file '%s' saved"), path.c_str());
}

void
WaypointGlue::SaveWaypoint(const Waypoint &wp)
{
  const auto path = LocalPath(_T("user.cup"));

  FileOutputStream file(path, FileOutputStream::Mode::APPEND_OR_CREATE);
  BufferedOutputStream writer(file);

  WriteCup(writer, wp);

  writer.Flush();
  file.Commit();
}
