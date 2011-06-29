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

#define CtrlC 0x03

namespace CAI302 {

#pragma pack(push, 1) // force byte alignment

  /** Structure for CAI302 glider response */
  struct PolarMeta {
    unsigned char result[3];
    unsigned char record_size;
  } gcc_packed;

  /** Structure for CAI302 glider data */
  struct Polar {
    unsigned char result[3];
    unsigned char glider_type[12];
    unsigned char glider_id[12];
    unsigned char best_ld;
    unsigned char best_glide_speed;
    unsigned char two_ms_sink_at_speed;
    unsigned char reserved1;
    unsigned short weight_in_litres;
    unsigned short ballast_capacity;
    unsigned short reserved2;
    unsigned short config_word; // locked(1) = FF FE.  unlocked(0) = FF FF
    unsigned short wing_area; // 100ths square meters
    unsigned char  Spare[60]; // 302 expect more data than the documented filed
    // be shure there is space to hold the data
  } gcc_packed;

  /** Structure for CAI302 Odata info */
  struct PilotMeta {
    unsigned char result[3];
    unsigned char count;
    unsigned char record_size;
  } gcc_packed;

  /** Structure for CAI302 settings */
  struct Pilot {
    unsigned char  result[3];
    char name[24];
    unsigned char old_units; // old unit
    unsigned char old_temperatur_units; // 0 = Celcius, 1 = Farenheight
    unsigned char sink_tone;
    unsigned char total_energy_final_glide;
    unsigned char show_final_glide_altitude_difference;
    unsigned char map_datum; // ignored on IGC version
    unsigned short approach_radius;
    unsigned short arrival_radius;
    unsigned short enroute_logging_interval;
    unsigned short close_logging_interval;
    unsigned short time_between_flight_logs; // [Minutes]
    unsigned short minimum_speed_to_force_flight_logging; // (Knots)
    unsigned char stf_dead_band; // (10ths M/S)
    unsigned char reserved_vario; // multiplexed w/ vario mode:
    // Tot Energy, SuperNetto, Netto
    unsigned short unit_word;
    unsigned short reserved2;
    unsigned short margin_height; // (10ths of Meters)
    unsigned char spare[60]; // 302 expect more data than the documented filed
    // be shure there is space to hold the data
  } gcc_packed;

  /** Structure for CAI302 device info */
  struct GeneralInfo {
    unsigned char result[3];
    unsigned char reserved[15];
    unsigned char id[3];
    unsigned char type;
    unsigned char version[5];
    unsigned char reserved2[5];
    unsigned char cai302_id;
    unsigned char reserved3[2];
  } gcc_packed;

#pragma pack(pop)
}

#endif
