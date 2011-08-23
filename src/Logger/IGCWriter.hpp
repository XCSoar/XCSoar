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
struct NMEAInfo;
struct Declaration;
struct GeoPoint;

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
   * (NMEA_INFO.Simulator), then this flag is true, and signing is
   * disabled.
   */
  bool Simulator;

  struct LogPoint_GPSPosition {
    bool Initialized;

    GeoPoint Location;
    int GPSAltitude;

    const LogPoint_GPSPosition &operator=(const NMEAInfo &gps_info);
  };

  LogPoint_GPSPosition LastValidPoint;

public:
  IGCWriter(const TCHAR *_path, const NMEAInfo &gps_info);

  bool flush();
  void finish(const NMEAInfo &gps_info);
  void sign();

  bool writeln(const char *line);

private:
  bool write_tstring(const char *a, const TCHAR *b);

  static const char *GetHFFXARecord();
  static const char *GetIRecord();
  static fixed GetEPE(const NMEAInfo &gps_info);
  /** Satellites in use if logger fix quality is a valid gps */
  static int GetSIU(const NMEAInfo &gps_info);

public:
  void header(const BrokenDateTime &DateTime,
              const TCHAR *pilot_name, const TCHAR *aircraft_model,
              const TCHAR *aircraft_registration,
              const TCHAR *strAssetNumber, const TCHAR *driver_name);

  void AddDeclaration(const GeoPoint &location, const TCHAR *ID);
  void StartDeclaration(const BrokenDateTime &FirstDateTime,
                        const int number_of_turnpoints);
  void EndDeclaration(void);

  void LoggerNote(const TCHAR *text);

  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const NMEAInfo &gps_info, const char *event);
};

#endif
