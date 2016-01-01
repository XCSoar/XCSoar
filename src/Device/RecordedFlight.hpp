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

#ifndef XCSOAR_DEVICE_RECORDED_FLIGHT_HPP
#define XCSOAR_DEVICE_RECORDED_FLIGHT_HPP

#include "Util/StaticArray.hxx"
#include "FlightInfo.hpp"

#include <stdint.h>

struct RecordedFlightInfo : FlightInfo {
  /**
   * Optional driver specific data to address a flight.
   */
  union {
    /**
     * Flight number, used by the CAI302 driver.
     */
    uint8_t cai302;

    /**
     * Flight address, used by the IMI ERIXX driver.
     */
    uint32_t imi;

    struct {
      /**
       * File name.  Only used by the LXNAV Nano sub-driver.  If this
       * is empty, then the "classic" Colibri protocol is used.
       */
      char nano_filename[16];

      uint8_t start_address[3];
      uint8_t end_address[3];
    } lx;

    /**
     * Flight number, used by the FLARM driver.
     */
    uint8_t flarm;

    /**
     * Flight number, used by Volkslogger driver
     */
    uint8_t volkslogger;

    /**
     * Flight number, used by the Flytec driver.
     */
    unsigned flytec;
  } internal;
};

class RecordedFlightList : public StaticArray<RecordedFlightInfo, 128u> {
};

#endif
