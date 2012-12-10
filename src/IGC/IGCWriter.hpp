/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Util/BatchBuffer.hpp"
#include "Math/fixed.hpp"
#include "IGCFix.hpp"
#include "IO/TextWriter.hpp"

#include <tchar.h>

struct GPSState;
struct BrokenDateTime;
struct NMEAInfo;
struct Declaration;
struct GeoPoint;

class IGCWriter {
  enum {
    /** Number of records in disk buffer */
    LOGGER_DISK_BUFFER_NUM_RECS = 10,
    MAX_IGC_BUFF = 255,
  };

  TextWriter file;

  BatchBuffer<char[MAX_IGC_BUFF],LOGGER_DISK_BUFFER_NUM_RECS> buffer;

  GRecord grecord;

  IGCFix fix;

public:
  IGCWriter(const TCHAR *path);

  bool Flush();
  void Sign();

private:
  /**
   * Begin writing a new line.  The returned buffer has #MAX_IGC_BUFF
   * bytes.  Call CommitLine() when you are done writing to the buffer.
   *
   * @return nullptr on error
   */
  char *BeginLine();

  /**
   * Finish writing the line.
   *
   * @param line the buffer obtained with BeginLine()
   * @return true on success
   */
  bool CommitLine(char *line);

  bool WriteLine(const char *line);
  bool WriteLine(const char *a, const TCHAR *b);

  static const char *GetHFFXARecord();
  static const char *GetIRecord();
  static fixed GetEPE(const GPSState &gps);
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
