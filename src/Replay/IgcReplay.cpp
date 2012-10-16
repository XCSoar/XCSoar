/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Replay/IgcReplay.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IO/LineReader.hpp"
#include "NMEA/Info.hpp"

IgcReplay::IgcReplay(NLineReader *_reader)
  :AbstractReplay(),
   cli(fixed(0.98)),
   reader(_reader),
   t_simulation(fixed_zero)
{
  cli.Reset();
}

IgcReplay::~IgcReplay()
{
  delete reader;
}

bool
IgcReplay::ScanBuffer(const char *buffer, IGCFix &fix)
{
  return IGCParseFix(buffer, fix) && fix.gps_valid;
}

bool
IgcReplay::ReadPoint(IGCFix &fix)
{
  char *buffer;

  while ((buffer = reader->ReadLine()) != NULL) {
    if (ScanBuffer(buffer, fix))
      return true;
  }

  return false;
}

bool
IgcReplay::UpdateTime(fixed time_scale)
{
  const fixed t_simulation_last = t_simulation;

  t_simulation += fixed_one * time_scale;

  return (t_simulation > t_simulation_last);
}

bool
IgcReplay::Update(NMEAInfo &data, fixed time_scale)
{
  if (positive(t_simulation) && !UpdateTime(time_scale))
    return true;

  // if need a new point
  while (cli.NeedData(t_simulation)) {
    IGCFix fix;
    if (!ReadPoint(fix))
      return false;

    if (fix.pressure_altitude == 0 && fix.gps_altitude > 0)
      /* no pressure altitude was recorded - fall back to GPS
         altitude */
      fix.pressure_altitude = fix.gps_altitude;

    if (fix.time.Plausible())
      cli.Update(fixed(fix.time.GetSecondOfDay()), fix.location,
                 fixed(fix.gps_altitude), fixed(fix.pressure_altitude));
  }

  if (!positive(t_simulation))
    t_simulation = cli.GetMaxTime();

  const CatmullRomInterpolator::Record r = cli.Interpolate(t_simulation);
  const GeoVector v = cli.GetVector(t_simulation);

  data.clock = t_simulation;
  data.alive.Update(data.clock);
  data.ProvideTime(t_simulation);
  data.location = r.location;
  data.location_available.Update(data.clock);
  data.ground_speed = v.distance;
  data.ground_speed_available.Update(data.clock);
  data.track = v.bearing;
  data.track_available.Update(data.clock);
  data.gps_altitude = r.gps_altitude;
  data.gps_altitude_available.Update(data.clock);
  data.ProvidePressureAltitude(r.baro_altitude);
  data.ProvideBaroAltitudeTrue(r.baro_altitude);
  data.gps.real = false;
  data.gps.replay = true;
  data.gps.simulator = false;

  return true;
}
