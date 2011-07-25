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

#ifndef XCSOAR_DEVICE_PARSER_HPP
#define XCSOAR_DEVICE_PARSER_HPP

#include "Math/fixed.hpp"

struct FLARM_STATE;
struct NMEAInfo;
struct BrokenDateTime;
class NMEAInputLine;

class NMEAParser
{
public:
  static bool ignore_checksum;

private:
  static int StartDay;

  bool GGAAvailable;
  fixed LastTime;

public:
  bool real;

  bool use_geoid;

  bool isFlarm;

public:
  NMEAParser();
  void Reset(void);

  void SetReal(bool _real) {
    real = _real;
  }

  void DisableGeoid() {
    use_geoid = true;
  }

  bool ParseNMEAString_Internal(const char *line, NMEAInfo &info);

public:
  // these routines can be used by other parsers.

  /**
   * Reads an altitude value, and the unit from a second volumn.
   */
  static bool ReadAltitude(NMEAInputLine &line, fixed &value_r);

  static bool NMEAChecksum(const char *String);

private:

  /**
   * Verifies the given fix time.  If it is smaller than LastTime, but
   * within a certain tolerance, the LastTime is returned, otherwise
   * the specified time is returned without modification.
   *
   * This is used to reduce quirks when the time stamps in GPGGA and
   * GPRMC are off by a second.  Without this workaround, XCSoar loses
   * the GPS fix every now and then, because GPRMC is ignored most of
   * the time.
   */
  gcc_pure
  fixed TimeAdvanceTolerance(fixed time) const;

  bool TimeHasAdvanced(fixed ThisTime, NMEAInfo &info);
  static fixed TimeModify(fixed FixTime, BrokenDateTime &date_time,
                          bool date_available);

  bool GLL(NMEAInputLine &line, NMEAInfo &info);
  bool GGA(NMEAInputLine &line, NMEAInfo &info);
  bool GSA(NMEAInputLine &line, NMEAInfo &info);
  bool RMC(NMEAInputLine &line, NMEAInfo &info);
  static bool RMB(NMEAInputLine &line, NMEAInfo &info);
  bool RMZ(NMEAInputLine &line, NMEAInfo &info);

  static bool WP0(NMEAInputLine &line, NMEAInfo &info);
  static bool WP1(NMEAInputLine &line, NMEAInfo &info);
  static bool WP2(NMEAInputLine &line, NMEAInfo &info);

  // Additional sentences
  static bool PTAS1(NMEAInputLine &line, NMEAInfo &info); // RMN: Tasman instruments.  TAS, Vario, QNE-altitude

  // FLARM sentences
  bool PFLAU(NMEAInputLine &line, FLARM_STATE &flarm, fixed Time);
  bool PFLAA(NMEAInputLine &line, NMEAInfo &info);
};

#endif
