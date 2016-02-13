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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Declaration.hpp"
#include "Operation/Operation.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef _UNICODE
#include <windows.h>
#endif

static void
convert_string(char *dest, size_t size, const TCHAR *src)
{
#ifdef _UNICODE
  size_t length = _tcslen(src);
  if (length >= size)
    length = size - 1;

  int length2 = ::WideCharToMultiByte(CP_ACP, 0, src, length, dest, size,
                                      nullptr, nullptr);
  if (length2 < 0)
    length2 = 0;
  dest[length2] = '\0';
#else
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
#endif
}

static bool
cai302DeclAddWaypoint(Port &port, int DeclIndex, const Waypoint &way_point,
                      OperationEnvironment &env)
{
  char Name[13];
  convert_string(Name, sizeof(Name), way_point.name.c_str());

  return CAI302::DeclareTP(port, DeclIndex, way_point.location,
                           (int)way_point.elevation,
                           Name, env);
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             gcc_unused OperationEnvironment &env)
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

  if (!CAI302::DownloadMode(port, env))
    return false;

  convert_string(pilot.name, sizeof(pilot.name), declaration.pilot_name);
  if (!CAI302::DownloadPilot(port, pilot, 0, env))
    return false;

  env.SetProgressPosition(5);

  convert_string(polar.glider_type, sizeof(polar.glider_type),
                 declaration.aircraft_type);
  convert_string(polar.glider_id, sizeof(polar.glider_id),
                 declaration.aircraft_registration);
  if (!CAI302::DownloadPolar(port, polar, env))
    return false;

  env.SetProgressPosition(6);

  for (unsigned i = 0; i < size; ++i) {
    if (!cai302DeclAddWaypoint(port, i, declaration.GetWaypoint(i), env))
      return false;

    env.SetProgressPosition(7 + i);
  }

  return CAI302::DeclareSave(port, env);
}

bool
CAI302Device::Declare(const Declaration &declaration,
                      gcc_unused const Waypoint *home,
                      OperationEnvironment &env)
{
  if (!UploadMode(env))
    return false;

  if (!DeclareInner(port, declaration, env)) {
    mode = Mode::UNKNOWN;
    return false;
  }

  return true;
}
