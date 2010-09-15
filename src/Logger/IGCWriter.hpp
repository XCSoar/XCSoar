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

#ifndef XCSOAR_IGC_WRITER_HPP
#define XCSOAR_IGC_WRITER_HPP

#include "Logger/LoggerFRecord.hpp"
#include "Logger/LoggerGRecord.hpp"
#include "BatchBuffer.hpp"
#include "Math/fixed.hpp"
#include "Engine/Navigation/GeoPoint.hpp"

#include <tchar.h>
#include <windef.h> /* for MAX_PATH */

struct BrokenDateTime;
struct NMEA_INFO;
struct Declaration;
class GeoPoint;

class IGCWriter {
  enum {
    LOGGER_DISK_BUFFER_NUM_RECS = 10, /**< Number of records in disk buffer */
    MAX_IGC_BUFF = 255,
  };

  TCHAR path[MAX_PATH];
  BatchBuffer<char[MAX_IGC_BUFF],LOGGER_DISK_BUFFER_NUM_RECS> buffer;

  LoggerFRecord frecord;
  GRecord grecord;

  /**
   * If at least one GPS fix came from the simulator
   * (NMEA_INFO.Simulator), the this flag is true, and signing is
   * disabled.
   */
  bool Simulator;

  struct LogPoint_GPSPosition {
    bool Initialized;

    GeoPoint Location;
    int GPSAltitude;

    const LogPoint_GPSPosition &operator=(const NMEA_INFO &gps_info);
  };

  LogPoint_GPSPosition LastValidPoint;

public:
  IGCWriter(const TCHAR *_path, const NMEA_INFO &gps_info);

  bool flush();
  void finish(const NMEA_INFO &gps_info);
  void sign();

  bool writeln(const char *line);

private:
  bool write_tstring(const char *a, const TCHAR *b);

  static const char *GetHFFXARecord();
  static const char *GetIRecord();
  static fixed GetEPE(const NMEA_INFO &gps_info);
  static int GetSIU(const NMEA_INFO &gps_info);

public:
  void header(const BrokenDateTime &DateTime,
              const TCHAR *pilot_name, const TCHAR *aircraft_model,
              const TCHAR *aircraft_registration,
              const TCHAR *strAssetNumber, const TCHAR *driver_name);

  void AddDeclaration(const GeoPoint &location, const TCHAR *ID);
  void StartDeclaration(const BrokenDateTime &FirstDateTime,
                        const int numturnpoints);
  void EndDeclaration(void);

  void LoggerNote(const TCHAR *text);

  void LogPoint(const NMEA_INFO &gps_info);
  void LogEvent(const NMEA_INFO &gps_info, const char *event);
};

#endif
