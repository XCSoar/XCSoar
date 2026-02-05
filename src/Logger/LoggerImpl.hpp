// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LoggerFRecord.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/Stamp.hpp"
#include "Geo/GeoPoint.hpp"
#include "system/Path.hpp"
#include "util/OverwritingRingBuffer.hpp"

#include <memory>

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
    TimeStamp time;
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
  std::unique_ptr<IGCWriter> writer;

  OverwritingRingBuffer<PreTakeoffBuffer, PRETAKEOFF_BUFFER_MAX> pre_takeoff_buffer;

  LoggerFRecord frecord;

  /**
   * If at least one GPS fix came from the simulator
   * (NMEA_INFO.Simulator), then this flag is true, and signing is
   * disabled.
   */
  bool simulator;

public:
  LoggerImpl();
  ~LoggerImpl() noexcept;

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const NMEAInfo &gps_info, const char* event);

  bool IsActive() const noexcept {
    return writer != nullptr;
  }

  void StartLogger(const NMEAInfo &gps_info, const LoggerSettings &settings,
                   const char *asset_number, const Declaration &decl);

  /**
   * Stops the logger
   * @param gps_info NMEA_INFO struct holding the current date
   */
  void StopLogger(const NMEAInfo &gps_info);
  void LoggerNote(const char *text);
  void ClearBuffer() noexcept;

private:
  /**
   * @param logger_id the ID of the logger, consisting of exactly 3
   * alphanumeric characters (plain ASCII)
   */
  bool StartLogger(const NMEAInfo &gps_info, const LoggerSettings &settings,
                   const char *logger_id);

private:
  void LogPointToBuffer(const NMEAInfo &gps_info) noexcept;
  void WritePoint(const NMEAInfo &gps_info);
};
