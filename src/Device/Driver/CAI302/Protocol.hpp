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

#ifndef XCSOAR_CAI302_PROTOCOL_HPP
#define XCSOAR_CAI302_PROTOCOL_HPP

#include "Compiler.h"

#include <stdint.h>

#define CtrlC 0x03

namespace CAI302 {

#pragma pack(push, 1) // force byte alignment

  /** Structure for CAI302 glider response */
  struct PolarMeta {
    uint8_t result[3];
    uint8_t record_size;
  } gcc_packed;

  /** Structure for CAI302 glider data */
  struct Polar {
    uint8_t result[3];
    uint8_t glider_type[12];
    uint8_t glider_id[12];
    uint8_t best_ld;
    uint8_t best_glide_speed;
    uint8_t two_ms_sink_at_speed;
    uint8_t reserved1;
    uint16_t weight_in_litres;
    uint16_t ballast_capacity;
    uint16_t reserved2;
    uint16_t config_word; // locked(1) = FF FE.  unlocked(0) = FF FF
    uint16_t wing_area; // 100ths square meters
    uint8_t  Spare[60]; // 302 expect more data than the documented filed
    // be shure there is space to hold the data
  } gcc_packed;

  /** Structure for CAI302 Odata info */
  struct PilotMeta {
    uint8_t result[3];
    uint8_t count;
    uint8_t record_size;
  } gcc_packed;

  /** Structure for CAI302 settings */
  struct Pilot {
    uint8_t  result[3];
    char name[24];
    uint8_t old_units; // old unit
    uint8_t old_temperatur_units; // 0 = Celcius, 1 = Farenheight
    uint8_t sink_tone;
    uint8_t total_energy_final_glide;
    uint8_t show_final_glide_altitude_difference;
    uint8_t map_datum; // ignored on IGC version
    uint16_t approach_radius;
    uint16_t arrival_radius;
    uint16_t enroute_logging_interval;
    uint16_t close_logging_interval;
    uint16_t time_between_flight_logs; // [Minutes]
    uint16_t minimum_speed_to_force_flight_logging; // (Knots)
    uint8_t stf_dead_band; // (10ths M/S)
    uint8_t reserved_vario; // multiplexed w/ vario mode:
    // Tot Energy, SuperNetto, Netto
    uint16_t unit_word;
    uint16_t reserved2;
    uint16_t margin_height; // (10ths of Meters)
    uint8_t spare[60]; // 302 expect more data than the documented filed
    // be shure there is space to hold the data
  } gcc_packed;

  /** Structure for CAI302 device info */
  struct GeneralInfo {
    uint8_t result[3];
    uint8_t reserved[15];
    uint8_t id[3];
    uint8_t type;
    uint8_t version[5];
    uint8_t reserved2[5];
    uint8_t cai302_id;
    uint8_t reserved3[2];
  } gcc_packed;

#pragma pack(pop)
}

#endif
