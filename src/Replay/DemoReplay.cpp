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
#include "DemoReplay.hpp"

DemoReplay::DemoReplay():
  AbstractReplay(),
  parms(),
  autopilot(parms),
  aircraft()
{
}

void
DemoReplay::Start(const TaskAccessor& task, const GeoPoint& default_location)
{
  enabled = true;
  autopilot.set_default_location(default_location);
  autopilot.Start(task);
  aircraft.Start(autopilot.location_start, autopilot.location_previous,
                 parms.start_alt);
}

bool
DemoReplay::Update(TaskAccessor& task)
{
  autopilot.update_state(task, aircraft.GetState(), time_scale);
  aircraft.Update(autopilot.heading, time_scale);
  if (!autopilot.update_autopilot(task, aircraft.GetState(), aircraft.GetLastState())) {
    enabled = false;
    return false;
  }
  return true;
}

void
DemoReplay::Stop()
{
  autopilot.Stop();
  enabled = false;
  on_stop();
}

bool
DemoReplay::UpdateTime()
{
  // nothing yet
  return true;
}

void
DemoReplay::ResetTime()
{
  // nothing yet
}
