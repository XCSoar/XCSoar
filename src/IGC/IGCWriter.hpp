// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Logger/GRecord.hpp"
#include "IGCFix.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"

#include <array>
#include <string_view>

#include <tchar.h>

class Path;
struct GPSState;
struct BrokenDateTime;
struct NMEAInfo;
struct GeoPoint;

class IGCWriter {
  FileOutputStream file;
  BufferedOutputStream buffered;

  GRecord grecord;

  IGCFix fix;

  std::array<char, 255> buffer;

public:
  /**
   * Throws on error.
   */
  explicit IGCWriter(Path path);

  void Flush() {
    buffered.Flush();
  }

  void Sign();

private:
  /**
   * Finish writing the line.
   */
  void CommitLine(std::string_view line);

  void WriteLine(const char *line);
  void WriteLine(const char *a, const char *b);

  static const char *GetHFFXARecord();
  static const char *GetIRecord();
  static double GetEPE(const GPSState &gps);
  /** Satellites in use if logger fix quality is a valid gps */
  static int GetSIU(const GPSState &gps);

public:
  /**
   * @param logger_id the ID of the logger, consisting of exactly 3
   * alphanumeric characters (plain ASCII)
   */
  void WriteHeader(const BrokenDateTime &date_time,
                   const char *pilot_name,
                   const char *copilot_name,
                   const char *aircraft_model,
                   const char *aircraft_registration,
                   const char *competition_id,
                   const char *logger_id, const char *driver_name,
                   bool simulator);

  void AddDeclaration(const GeoPoint &location, const char *ID);
  void StartDeclaration(const BrokenDateTime &date_time,
                        const int number_of_turnpoints);
  void EndDeclaration();

  void LoggerNote(const char *text);

  void LogPoint(const IGCFix &fix, int epe, int satellites);
  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const IGCFix &fix, int epe, int satellites, const char *event);
  void LogEvent(const NMEAInfo &gps_info, const char *event);

  void LogEmptyFRecord(const BrokenTime &time);
  void LogFRecord(const BrokenTime &time, const int *satellite_ids);

protected:
  void LogEvent(const BrokenTime &time, const char *event = "");
};
