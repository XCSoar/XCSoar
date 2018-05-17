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

#if !defined(XCSOAR_LOGGER_IMPL_HPP)
#define XCSOAR_LOGGER_IMPL_HPP

#include "LoggerFRecord.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include "OS/Path.hpp"
#include "Util/OverwritingRingBuffer.hpp"

#include <tchar.h>

struct NMEAInfo;
struct LoggerSettings;
struct Declaration;
class IGCWriter;

/**
 * Implementation of logger
 */
class LoggerImpl
{
public:
  enum {
    /** Buffer size (s) of points recorded before takeoff */
    PRETAKEOFF_BUFFER_MAX = 60,
  };

  /** Buffer for points recorded before takeoff */
  struct PreTakeoffBuffer
  {
    /** Location of fix */
    GeoPoint location;
    /** Barometric altitude (m STD) */
    double pressure_altitude;
    /** GPS Altitude (m) */
    double altitude_gps;
    /** Date and time of fix */
    BrokenDateTime date_time_utc;
    /** IDs of satellites in fix */
    int satellite_ids[GPSState::MAXSATELLITES];
    bool satellite_ids_available;
    /** Time of fix (s) */
    double time;
    /** GPS fix quality */
    FixQuality fix_quality;
    /** GPS fix state */
    int satellites_used;
    bool satellites_used_available;
    /** GPS Horizontal Dilution of precision */
    double hdop;

    /**
     * Is the fix real? (no replay, no simulator)
     */
    bool real;

    bool pressure_altitude_available;
    bool gps_altitude_available;

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
  AllocatedPath filename;
  IGCWriter *writer;

  OverwritingRingBuffer<PreTakeoffBuffer, PRETAKEOFF_BUFFER_MAX> pre_takeoff_buffer;

  LoggerFRecord frecord;

  /**
   * If at least one GPS fix came from the simulator
   * (NMEA_INFO.Simulator), then this flag is true, and signing is
   * disabled.
   */
  bool simulator;

public:
  /** Default constructor */
  LoggerImpl();
  ~LoggerImpl();

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const NMEAInfo &gps_info, const char* event);

  bool IsActive() const {
    return writer != nullptr;
  }

  void StartLogger(const NMEAInfo &gps_info, const LoggerSettings &settings,
                   const TCHAR *asset_number, const Declaration &decl);

  /**
   * Stops the logger
   * @param gps_info NMEA_INFO struct holding the current date
   */
  void StopLogger(const NMEAInfo &gps_info);
  void LoggerNote(const TCHAR *text);
  void ClearBuffer();

private:
  /**
   * @param logger_id the ID of the logger, consisting of exactly 3
   * alphanumeric characters (plain ASCII)
   */
  bool StartLogger(const NMEAInfo &gps_info, const LoggerSettings &settings,
                   const char *logger_id);
  
private:
  void LogPointToBuffer(const NMEAInfo &gps_info);
  void WritePoint(const NMEAInfo &gps_info);
};

#endif
