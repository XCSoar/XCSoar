/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <tchar.h>

struct FLARM_STATE;
struct NMEA_INFO;
struct BrokenDateTime;
class NMEAInputLine;

class NMEAParser
{
public:
  NMEAParser();
  void Reset(void);

  bool ParseNMEAString_Internal(const TCHAR *String, NMEA_INFO *GPS_INFO);
  bool gpsValid;

  bool activeGPS;
  bool isFlarm;

  static int StartDay;

public:
  void TestRoutine(NMEA_INFO *GPS_INFO);

  // these routines can be used by other parsers.

  /**
   * Reads an altitude value, and the unit from a second volumn.
   */
  static bool ReadAltitude(NMEAInputLine &line, fixed &value_r);

  static bool NMEAChecksum(const TCHAR *String);

private:
  bool GSAAvailable;
  bool GGAAvailable;
  bool RMZAvailable;
  bool RMAAvailable;
  fixed RMZAltitude;
  fixed RMAAltitude;
  double LastTime;

  bool TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO);
  static double TimeModify(double FixTime, BrokenDateTime &date_time);

  bool GLL(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool GGA(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool GSA(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool RMC(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool RMB(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool RMA(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool RMZ(NMEAInputLine &line, NMEA_INFO *GPS_INFO);

  bool WP0(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool WP1(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
  bool WP2(NMEAInputLine &line, NMEA_INFO *GPS_INFO);

  // Additional sentences
  bool PTAS1(NMEAInputLine &line, NMEA_INFO *GPS_INFO); // RMN: Tasman instruments.  TAS, Vario, QNE-altitude

  // FLARM sentences
  bool PFLAU(NMEAInputLine &line, FLARM_STATE &flarm);
  bool PFLAA(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
};

#endif
