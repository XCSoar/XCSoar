// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"
#include "FlightInfo.hpp"

#include <cstdint>

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

    /**
     * Flight id and igc file size, used by the LX Eos driver.
     */
    struct {
      uint16_t flight_id;
      uint32_t file_size;
    } lx_eos;
    
  } internal;
};

class RecordedFlightList : public StaticArray<RecordedFlightInfo, 128u> {
};
