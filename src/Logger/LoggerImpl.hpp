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
    PRETAKEOFF_BUFFER_MAX = 60,
    /**< Number of records in disk buffer */
    DISK_BUFFER_NUM_RECS = 10,
    MAX_IGC_BUFF = 255,
  };

  /** Buffer for points recorded before takeoff */
  struct PreTakeoffBuffer
  {
    /** Location of fix */
    GeoPoint location;
    /** GPS Altitude (m) */
    fixed altitude_gps;
    /** Barometric altitude (m) */
    fixed altitude_baro;
    /** Date and time of fix */
    BrokenDateTime date_time_utc;
    /** IDs of satellites in fix */
    int satellite_ids[GPSState::MAXSATELLITES];
    /** Time of fix (s) */
    fixed time;
    /** GPS fix state */
    int nav_warning;
    /** GPS fix quality */
    int fix_quality;
    /** GPS fix state */
    int satellites_used;
    /** GPS Horizontal Dilution of precision */
    fixed hdop;

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
    const struct PreTakeoffBuffer &operator=(const NMEAInfo &src);
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

  bool IsActive() const {
    return writer != NULL;
  }

  /**
   * Deletes old IGC files until at least LOGGER_MINFREESTORAGE KiB of space are
   * available
   * @param gps_info Current NMEA_INFO
   * @return True if enough space could be cleared, False otherwise
   */
  static bool LoggerClearFreeSpace(const NMEAInfo &gps_info);
  void StartLogger(const NMEAInfo &gps_info, const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber, const Declaration &decl);

  /**
   * Stops the logger
   * @param gps_info NMEA_INFO struct holding the current date
   */
  void StopLogger(const NMEAInfo &gps_info);
  void LoggerNote(const TCHAR *text);
  void ClearBuffer();

private:
  void StartLogger(const NMEAInfo &gps_info, const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber);
  
private:
  void LogPointToBuffer(const NMEAInfo &gps_info);

private:
  TCHAR filename[MAX_PATH];
  OverwritingRingBuffer<PreTakeoffBuffer, PRETAKEOFF_BUFFER_MAX> pre_takeoff_buffer;
};

#endif
