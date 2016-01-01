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

#ifndef XCSOAR_DEVICE_DRIVER_LX_LXN_HPP
#define XCSOAR_DEVICE_DRIVER_LX_LXN_HPP

#include "Compiler.h"

#include <stdint.h>

/**
 * Definitions for the LXN file format.
 */
namespace LXN {
  enum Command {
    EMPTY = 0x00,
    END = 0x40,
    VERSION = 0x7f,
    START = 0x80,
    ORIGIN = 0xa0,
    SECURITY_OLD = 0xf5,
    SERIAL = 0xf6,
    POSITION_OK = 0xbf,
    POSITION_BAD = 0xc3,
    SECURITY_7000 = 0xef,
    SECURITY = 0xf0,
    COMPETITION_CLASS = 0xf1,
    EVENT = 0xf4,
    TASK = 0xf7,
    B_EXT = 0xf9,
    K_EXT = 0xfa,
    DATE = 0xfb,
    FLIGHT_INFO = 0xfc,
    K_EXT_CONFIG = 0xfe, /* 'J': extensions in the 'K' record */
    B_EXT_CONFIG = 0xff, /* 'I': extensions to the 'B' record */
  };

  enum SecurityType {
    SECURITY_LOW = 0x0d,
    SECURITY_MED = 0x0e,
    SECURITY_HIGH = 0x0f,
  };

#pragma pack(push, 1) // force byte alignment

  struct String {
    uint8_t length;
    char value[0];
  } gcc_packed;

  struct End {
    uint8_t cmd;
  } gcc_packed;

  struct Version {
    uint8_t cmd;
    uint8_t hardware, software;
  } gcc_packed;

  struct Start {
    uint8_t cmd;
    char streraz[8];
    uint8_t flight_no;
  } gcc_packed;

  struct Origin {
    uint8_t cmd;
    uint32_t time, latitude, longitude;
  } gcc_packed;

  struct SecurityOld {
    uint8_t cmd;
    char foo[22];
  } gcc_packed;

  struct Serial {
    uint8_t cmd;
    char serial[9];
  } gcc_packed;

  struct Position {
    uint8_t cmd;
    uint16_t time, latitude, longitude, aalt, galt;
  } gcc_packed;

  struct Security {
    uint8_t cmd;
    uint8_t length, type;
    uint8_t foo[64];
  } gcc_packed;

  /**
   * The G record found on LXN files downloaded from "LX 7000 pro
   * IGC".
   *
   * Reverse engineered.
   *
   * @see #Command::SECURITY_7000
   */
  struct Security7000 {
    uint8_t cmd;
    uint8_t x40;
    uint8_t line1[31], line2[32], line3[3];
    uint8_t zero[0x40];
  } gcc_packed;

  struct CompetitionClass {
    uint8_t cmd;
    char class_id[9];
  } gcc_packed;

  struct Event {
    uint8_t cmd;
    char foo[9];
  } gcc_packed;

  struct Task {
    uint8_t cmd;
    uint32_t time;
    uint8_t day, month, year;
    uint8_t day2, month2, year2;
    uint16_t task_id;
    int8_t num_tps;
    uint8_t usage[12];
    uint32_t longitude[12], latitude[12];
    char name[12][9];
  } gcc_packed;

  struct BExt {
    uint8_t cmd;
    uint16_t data[0];
  } gcc_packed;

  struct KExt {
    uint8_t cmd;
    uint8_t foo;
    uint16_t data[0];
  } gcc_packed;

  struct Date {
    uint8_t cmd;
    uint8_t day, month;
    uint16_t year;
  } gcc_packed;

  struct FlightInfo {
    uint8_t cmd;
    uint16_t id;
    char pilot[19];
    char glider[12];
    char registration[8];
    char competition_class[4];
    uint8_t competition_class_id;
    char observer[10];
    uint8_t gps_date;
    uint8_t fix_accuracy;
    char gps[60];
  } gcc_packed;

  struct ExtConfig {
    uint8_t cmd;
    uint16_t time, dat;
  } gcc_packed;

  struct ExtensionDefinition {
    char name[4];
    unsigned width;
  };

  struct ExtensionConfig {
    unsigned num;
    struct ExtensionDefinition extensions[16];
  };

  union Packet {
    const uint8_t *cmd;
    const struct String *string;
    const struct End *end;
    const struct Version *version;
    const struct Start *start;
    const struct Origin *origin;
    const struct SecurityOld *security_old;
    const struct Serial *serial;
    const struct Position *position;
    const Security7000 *security_7000;
    const struct Security *security;
    const struct CompetitionClass *competition_class;
    const struct Event *event;
    const struct Task *task;
    const struct BExt *b_ext;
    const struct KExt *k_ext;
    const struct Date *date;
    const struct FlightInfo *flight_info;
    const struct ExtConfig *ext_config;
  };

#pragma pack(pop)

  extern const struct ExtensionDefinition extension_defs[16];

  gcc_const
  const char *
  FormatGPSDate(unsigned gps_date);

  gcc_const
  const char *
  FormatCompetitionClass(unsigned class_id);
}

#endif
