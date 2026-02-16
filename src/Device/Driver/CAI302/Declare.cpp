// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Declaration.hpp"
#include "Operation/Operation.hpp"

#include <tchar.h>
#include <stdio.h>

static void
convert_string(char *dest, size_t size, const char *src)
{
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

static void
cai302DeclAddWaypoint(Port &port, int DeclIndex, const Waypoint &way_point,
                      OperationEnvironment &env)
{
  char Name[13];
  convert_string(Name, sizeof(Name), way_point.name.c_str());

  CAI302::DeclareTP(port, DeclIndex, way_point.location,
                    (int)way_point.GetElevationOrZero(),
                    Name, env);
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             [[maybe_unused]] OperationEnvironment &env)
{
  using CAI302::UploadShort;
  unsigned size = declaration.Size();

  env.SetProgressRange(6 + size);
  env.SetProgressPosition(0);

  CAI302::PilotMeta pilot_meta;
  if (!CAI302::UploadPilotMeta(port, pilot_meta, env))
    return false;

  env.SetProgressPosition(1);

  CAI302::Pilot pilot;
  if (!CAI302::UploadPilot(port, 0, pilot, env))
    return false;

  env.SetProgressPosition(2);

  CAI302::PolarMeta polar_meta;
  if (!CAI302::UploadPolarMeta(port, polar_meta, env))
    return false;

  env.SetProgressPosition(3);

  CAI302::Polar polar;
  if (!CAI302::UploadPolar(port, polar, env))
    return false;

  env.SetProgressPosition(4);

  CAI302::DownloadMode(port, env);

  convert_string(pilot.name, sizeof(pilot.name), declaration.pilot_name);
  CAI302::DownloadPilot(port, pilot, 0, env);

  env.SetProgressPosition(5);

  convert_string(polar.glider_type, sizeof(polar.glider_type),
                 declaration.aircraft_type);
  convert_string(polar.glider_id, sizeof(polar.glider_id),
                 declaration.aircraft_registration);
  CAI302::DownloadPolar(port, polar, env);

  env.SetProgressPosition(6);

  for (unsigned i = 0; i < size; ++i) {
    cai302DeclAddWaypoint(port, i, declaration.GetWaypoint(i), env);
    env.SetProgressPosition(7 + i);
  }

  CAI302::DeclareSave(port, env);
  return true;
}

bool
CAI302Device::Declare(const Declaration &declaration,
                      [[maybe_unused]] const Waypoint *home,
                      OperationEnvironment &env)
{
  UploadMode(env);

  if (!DeclareInner(port, declaration, env)) {
    mode = Mode::UNKNOWN;
    return false;
  }

  return true;
}
