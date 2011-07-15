/*
Copyright_License {

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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Port.hpp"
#include "Operation.hpp"
#include "Units/Units.hpp"
#include "OS/ByteOrder.hpp"

#include <tchar.h>
#include <stdio.h>
#include <assert.h>

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
                                      NULL, NULL);
  if (length2 < 0)
    length2 = 0;
  dest[length2] = '\0';
#else
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
#endif
}

static bool
cai302DeclAddWaypoint(Port &port, int DeclIndex, const Waypoint &way_point)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  tmp = way_point.Location.Latitude.value_degrees();
  NoS = 'N';
  if (tmp < 0) {
    NoS = 'S';
    tmp = -tmp;
  }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60;

  tmp = way_point.Location.Longitude.value_degrees();
  EoW = 'E';
  if (tmp < 0) {
    EoW = 'W';
    tmp = -tmp;
  }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60;

  char Name[13];
  convert_string(Name, sizeof(Name), way_point.Name.c_str());

  char szTmp[128];
  sprintf(szTmp, "D,%d,%02d%07.4f%c,%03d%07.4f%c,%s,%d\r",
          DeclIndex,
          DegLat, MinLat, NoS,
          DegLon, MinLon, EoW,
          Name,
          (int)way_point.Altitude);

  return CAI302::DownloadCommand(port, szTmp);
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             gcc_unused OperationEnvironment &env)
{
  using CAI302::UploadShort;
  using CAI302::DownloadCommand;
  unsigned size = declaration.size();

  port.SetRxTimeout(500);

  env.SetProgressRange(6 + size);
  env.SetProgressPosition(0);

  CAI302::CommandModeQuick(port);
  if (!CAI302::UploadMode(port) || env.IsCancelled())
    return false;

  port.SetRxTimeout(1500);

  CAI302::PilotMeta pilot_meta;
  if (!CAI302::UploadPilotMeta(port, pilot_meta) || env.IsCancelled())
    return false;

  env.SetProgressPosition(1);

  CAI302::Pilot pilot;
  if (!CAI302::UploadPilot(port, 0, pilot) || env.IsCancelled())
    return false;

  env.SetProgressPosition(2);

  CAI302::PolarMeta polar_meta;
  if (!CAI302::UploadPolarMeta(port, polar_meta) || env.IsCancelled())
    return false;

  env.SetProgressPosition(3);

  CAI302::Polar polar;
  if (!CAI302::UploadPolar(port, polar) || env.IsCancelled())
    return false;

  env.SetProgressPosition(4);

  if (!CAI302::DownloadMode(port) || env.IsCancelled())
    return false;

  char PilotName[25], GliderType[13], GliderID[13];
  convert_string(PilotName, sizeof(PilotName), declaration.PilotName);
  convert_string(GliderType, sizeof(GliderType), declaration.AircraftType);
  convert_string(GliderID, sizeof(GliderID), declaration.AircraftReg);

  char szTmp[255];
  sprintf(szTmp, "O,%-24s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
          PilotName,
          pilot.old_units,
          pilot.old_temperatur_units,
          pilot.sink_tone,
          pilot.total_energy_final_glide,
          pilot.show_final_glide_altitude_difference,
          pilot.map_datum,
          FromLE16(pilot.approach_radius),
          FromLE16(pilot.arrival_radius),
          FromLE16(pilot.enroute_logging_interval),
          FromLE16(pilot.close_logging_interval),
          FromLE16(pilot.time_between_flight_logs),
          FromLE16(pilot.minimum_speed_to_force_flight_logging),
          pilot.stf_dead_band,
          pilot.reserved_vario,
          FromLE16(pilot.unit_word),
          FromLE16(pilot.margin_height));

  if (!DownloadCommand(port, szTmp) || env.IsCancelled())
    return false;

  env.SetProgressPosition(5);

  sprintf(szTmp, "G,%-12s,%-12s,%d,%d,%d,%d,%d,%d,%d,%d\r",
          GliderType,
          GliderID,
          polar.best_ld,
          polar.best_glide_speed,
          polar.two_ms_sink_at_speed,
          FromLE16(polar.weight_in_litres),
          FromLE16(polar.ballast_capacity),
          0,
          FromLE16(polar.config_word),
          FromLE16(polar.wing_area));

  if (!DownloadCommand(port, szTmp) || env.IsCancelled())
    return false;

  env.SetProgressPosition(6);

  for (unsigned i = 0; i < size; ++i) {
    if (!cai302DeclAddWaypoint(port, 128 + i, declaration.get_waypoint(i)) ||
        env.IsCancelled())
      return false;

    env.SetProgressPosition(7 + i);
  }

  port.SetRxTimeout(1500); // D,255 takes more than 800ms
  return DownloadCommand(port, "D,255\r");
}

bool
CAI302Device::Declare(const Declaration &declaration,
                      OperationEnvironment &env)
{
  bool success = DeclareInner(*port, declaration, env);

  port->SetRxTimeout(500);

  if (success)
    CAI302::LogMode(*port);
  else
    CAI302::LogModeQuick(*port);

  return success;
}
