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
   * A file the Resume handler has validated and wants continued, or nullptr.
   * Consumed and cleared by StartLogger().
   */
  AllocatedPath resume_target = nullptr;

  /** Did the current Session continue an existing file? */
  bool appending = false;

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

  /**
   * Continue this file instead of opening a new one, for the next Logger
   * Session only.
   *
   * Set by the Resume handler once it has replayed a Cut Session and
   * confirmed the Flight was still airborne when it ended.  The logger never
   * makes that judgement itself: appending is a commitment that cannot be
   * undone once the two Flights share a file, so nothing is appended unless
   * the reconstruction was validated first.
   *
   * Consumed by the next StartLogger() and cleared there, so it can never
   * affect a later Session.
   */
  void SetResumeTarget(Path path) noexcept {
    resume_target = path;
  }

  bool IsResuming() const noexcept {
    return resume_target != nullptr;
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
