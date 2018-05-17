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

#ifndef XCSOAR_LOG_COMPUTER_HPP
#define XCSOAR_LOG_COMPUTER_HPP

#include "Geo/GeoPoint.hpp"
#include "Time/GPSClock.hpp"

#include <assert.h>

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct LoggerSettings;
class Logger;

class LogComputer {
  GeoPoint last_location;

  GPSClock log_clock;

  /** number of points to log at high rate */
  unsigned fast_log_num;

  Logger *logger;

public:
  LogComputer();

  void SetLogger(Logger *_logger) {
    assert(logger == nullptr);
    assert(_logger != nullptr);

    logger = _logger;
  }

  void Reset();
  void StartTask(const NMEAInfo &basic);
  bool Run(const MoreData &basic, const DerivedInfo &calculated,
           const LoggerSettings &settings_logger);

  void SetFastLogging() {
    fast_log_num = 5;
  }
};

#endif
