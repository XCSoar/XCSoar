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

#ifndef XCSOAR_IGC_WRITER_HPP
#define XCSOAR_IGC_WRITER_HPP

#include "Logger/GRecord.hpp"
#include "IGCFix.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"

#include <tchar.h>

class Path;
struct GPSState;
struct BrokenDateTime;
struct NMEAInfo;
struct GeoPoint;

class IGCWriter {
  enum {
    MAX_IGC_BUFF = 255,
  };

  FileOutputStream file;
  BufferedOutputStream buffered;

  GRecord grecord;

  IGCFix fix;

  char buffer[MAX_IGC_BUFF];

public:
  /**
   * Create a new IGC file.  The caller must check IsOpen().
   */
  explicit IGCWriter(Path path);

  void Flush() {
    buffered.Flush();
  }

  void Sign();

private:
  /**
   * Begin writing a new line.  The returned buffer has #MAX_IGC_BUFF
   * bytes.  Call CommitLine() when you are done writing to the buffer.
   *
   * @return nullptr on error
   */
  char *BeginLine() {
    return buffer;
  }

  /**
   * Finish writing the line.
   *
   * @param line the buffer obtained with BeginLine()
   */
  void CommitLine(char *line);

  void WriteLine(const char *line);
  void WriteLine(const char *a, const TCHAR *b);

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
                   const TCHAR *pilot_name, const TCHAR *aircraft_model,
                   const TCHAR *aircraft_registration,
                   const TCHAR *competition_id,
                   const char *logger_id, const TCHAR *driver_name,
                   bool simulator);

  void AddDeclaration(const GeoPoint &location, const TCHAR *ID);
  void StartDeclaration(const BrokenDateTime &date_time,
                        const int number_of_turnpoints);
  void EndDeclaration();

  void LoggerNote(const TCHAR *text);

  void LogPoint(const IGCFix &fix, int epe, int satellites);
  void LogPoint(const NMEAInfo &gps_info);
  void LogEvent(const IGCFix &fix, int epe, int satellites, const char *event);
  void LogEvent(const NMEAInfo &gps_info, const char *event);

  void LogEmptyFRecord(const BrokenTime &time);
  void LogFRecord(const BrokenTime &time, const int *satellite_ids);

protected:
  void LogEvent(const BrokenTime &time, const char *event = "");
};

#endif
