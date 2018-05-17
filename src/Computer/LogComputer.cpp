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

#include "LogComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Logger/Settings.hpp"
#include "Logger/Logger.hpp"

LogComputer::LogComputer()
  :logger(NULL) {}

void
LogComputer::Reset()
{
  last_location = GeoPoint::Invalid();
  fast_log_num = 0;
}

void
LogComputer::StartTask(const NMEAInfo &basic)
{
  if (logger != NULL)
    logger->LogStartEvent(basic);
}

bool
LogComputer::Run(const MoreData &basic, const DerivedInfo &calculated,
                 const LoggerSettings &settings_logger)
{
  const bool location_jump = basic.location_available &&
    last_location.IsValid() &&
    basic.location.DistanceS(last_location) > 200;

  last_location = basic.location_available
    ? basic.location : GeoPoint::Invalid();

  if (location_jump)
    // prevent bad fixes from being logged
    return false;

  // log points more often in circling mode
  unsigned period;
  if (fast_log_num) {
    period = 1;
    fast_log_num--;
  } else
    period = calculated.circling
      ? settings_logger.time_step_circling
      : settings_logger.time_step_cruise;

  if (log_clock.CheckAdvance(basic.time, period) && logger != nullptr)
      logger->LogPoint(basic);

  return true;
}
