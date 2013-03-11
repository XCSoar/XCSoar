/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Replay.hpp"
#include "IgcReplay.hpp"
#include "NmeaReplay.hpp"
#include "DemoReplayGlue.hpp"
#include "Util/StringUtil.hpp"
#include "OS/PathName.hpp"
#include "IO/FileLineReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "CatmullRomInterpolator.hpp"

#include <assert.h>

void
Replay::Stop()
{
  if (replay == NULL)
    return;

  delete replay;
  replay = NULL;

  delete cli;
  cli = nullptr;

  device_blackboard->StopReplay();

  if (logger != NULL)
    logger->ClearBuffer();
}

bool
Replay::Start(const TCHAR *_path)
{
  assert(_path != NULL);

  /* make sure the old AbstractReplay instance has cleaned up before
     creating a new one */
  delete replay;
  replay = NULL;

  delete cli;
  cli = nullptr;

  _tcscpy(path, _path);

  if (StringIsEmpty(path)) {
    replay = new DemoReplayGlue(task_manager);
  } else if (MatchesExtension(path, _T(".igc"))) {
    auto reader = new FileLineReaderA(path);
    if (reader->error()) {
      delete reader;
      return false;
    }

    replay = new IgcReplay(reader);

    cli = new CatmullRomInterpolator(fixed(0.98));
    cli->Reset();
  } else {
    auto reader = new FileLineReaderA(path);
    if (reader->error()) {
      delete reader;
      return false;
    }

    replay = new NmeaReplay(reader,
                            CommonInterface::GetSystemSettings().devices[0]);
  }

  if (logger != NULL)
    logger->ClearBuffer();

  virtual_time = fixed(-1);
  next_data.Reset();

  return true;
}

bool
Replay::Update()
{
  if (replay == nullptr)
    return false;

  if (!positive(time_scale))
    return true;

  if (!negative(virtual_time)) {
    /* update the virtual time */
    assert(clock.IsDefined());
    virtual_time += clock.ElapsedUpdate() * time_scale / 1000;
  } else {
    /* if we ever received a valid time from the AbstractReplay, then
       virtual_time must be initialised */
    assert(!next_data.time_available);
  }

  if (cli == nullptr) {
    if (next_data.time_available && virtual_time < next_data.time)
      /* still not time to use next_data */
      return true;

    {
      ScopeLock protect(device_blackboard->mutex);
      device_blackboard->SetReplayState() = next_data;
      device_blackboard->ScheduleMerge();
    }

    while (true) {
      if (!replay->Update(next_data))
        return false;

      assert(!next_data.gps.real);

      if (next_data.time_available) {
        if (negative(virtual_time)) {
          virtual_time = next_data.time;
          clock.Update();
          break;
        }

        if (next_data.time >= virtual_time)
          break;
      }
    }
  } else {
    while (cli->NeedData(virtual_time)) {
      if (!replay->Update(next_data))
        return false;

      assert(!next_data.gps.real);

      if (next_data.time_available)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude,
                    next_data.pressure_altitude);
    }

    if (negative(virtual_time)) {
      virtual_time = cli->GetMaxTime();
      clock.Update();
    }

    const CatmullRomInterpolator::Record r = cli->Interpolate(virtual_time);
    const GeoVector v = cli->GetVector(virtual_time);

    NMEAInfo data = next_data;
    data.clock = virtual_time;
    data.alive.Update(data.clock);
    data.ProvideTime(virtual_time);
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

    {
      ScopeLock protect(device_blackboard->mutex);
      device_blackboard->SetReplayState() = data;
      device_blackboard->ScheduleMerge();
    }
  }

  return true;
}
