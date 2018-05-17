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

#ifndef XCSOAR_LOGGER_SETTINGS_HPP
#define XCSOAR_LOGGER_SETTINGS_HPP

#include "Util/StaticString.hxx"

#include <stdint.h>

/**
 * Logger settings
 */
struct LoggerSettings {
  /**
   * Enable the #FlightLogger?
   */
  bool enable_flight_logger;

  /**
   * Enable the #NMEALogger?
   */
  bool enable_nmea_logger;

  /** Logger interval in cruise mode */
  uint16_t time_step_cruise;

  /** Logger interval in circling mode */
  uint16_t time_step_circling;

  enum class AutoLogger: uint8_t {
    ON,
    START_ONLY,
    OFF,
  } auto_logger;

  StaticString<32> logger_id;

  StaticString<64> pilot_name;

  void SetDefaults();
};

#endif
