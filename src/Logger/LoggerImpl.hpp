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

#if !defined(XCSOAR_LOGGER_IMPL_HPP)
#define XCSOAR_LOGGER_IMPL_HPP

#include "Sizes.h"
#include "DateTime.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Logger/IGCWriter.hpp"
#include "OverwritingRingBuffer.hpp"
#include "BatchBuffer.hpp"
#include "NMEA/Info.hpp"

#include <tchar.h>
#include <windef.h>

struct NMEAInfo;
struct SETTINGS_COMPUTER;
struct Declaration;
class OrderedTask;
class DeviceDescriptor;

/**
 * Implementation of logger
 */
class LoggerImpl
{
public:
  enum {
    /**< Buffer size (s) of points recorded before takeoff */
    LOGGER_PRETAKEOFF_BUFFER_MAX = 60,
    /**< Number of records in disk buffer */
    LOGGER_DISK_BUFFER_NUM_RECS = 10,
    MAX_IGC_BUFF = 255,
  };

  /** Buffer for points recorded before takeoff */
  struct LoggerPreTakeoffBuffer
  {
    /** Location of fix */
    GeoPoint Location;
    /** GPS Altitude (m) */
    fixed Altitude;
    /** Barometric altitude (m) */
    fixed BaroAltitude;
    /** Date and time of fix */
    BrokenDateTime DateTime;
    /** IDs of satellites in fix */
    int SatelliteIDs[GPSState::MAXSATELLITES];
    /** Time of fix (s) */
    fixed Time;
    /** GPS fix state */
    int NAVWarning;
    /** GPS fix quality */
    int FixQuality;
    /** GPS fix state */
    int SatellitesUsed;
    /** GPS Horizontal Dilution of precision */
    fixed HDOP;

    /**
     * Is the fix real? (no replay, no simulator)
     */
    bool real;

    /** 
     * Set buffer value from NMEA_INFO structure
     * 
     * @param src Item to set
     * 
     * @return Buffer value
     */
    const struct LoggerPreTakeoffBuffer &operator=(const NMEAInfo &src);
  };

private:
  IGCWriter *writer;

public:
  /** Default constructor */
  LoggerImpl();
  ~LoggerImpl();

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const NMEAInfo &gps_info, const char* event);

  bool isLoggerActive() const {
    return writer != NULL;
  }

  static bool LoggerClearFreeSpace(const NMEAInfo &gps_info);
  void StartLogger(const NMEAInfo &gps_info, const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber, const Declaration &decl);
  void StopLogger(const NMEAInfo &gps_info);
  void LoggerNote(const TCHAR *text);
  void clearBuffer();

private:
  void StartLogger(const NMEAInfo &gps_info, const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber);
  
private:
  void LogPointToBuffer(const NMEAInfo &gps_info);

private:
  TCHAR szLoggerFileName[MAX_PATH];
  OverwritingRingBuffer<LoggerPreTakeoffBuffer,LOGGER_PRETAKEOFF_BUFFER_MAX> PreTakeoffBuffer;
};

#endif
