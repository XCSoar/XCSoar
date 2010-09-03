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

#if !defined(XCSOAR_LOGGER_IMPL_HPP)
#define XCSOAR_LOGGER_IMPL_HPP

#include "Sizes.h"
#include "DateTime.hpp"
#include "GPSClock.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Logger/LoggerGRecord.hpp"
#include "OverwritingRingBuffer.hpp"
#include "BatchBuffer.hpp"

#include <tchar.h>
#include <windef.h>

struct NMEA_INFO;
struct SETTINGS_COMPUTER;
struct Declaration;
class OrderedTask;
class DeviceDescriptor;

/**
 * Implementation of logger
 */
class LoggerImpl {
public:
  enum {
    LOGGER_PRETAKEOFF_BUFFER_MAX = 60, /**< Buffer size (s) of points recorded before takeoff */
    LOGGER_DISK_BUFFER_NUM_RECS = 10, /**< Number of records in disk buffer */
    MAX_IGC_BUFF = 255,
  };

  /** Buffer for points recorded before takeoff */
  struct LoggerPreTakeoffBuffer {
    GEOPOINT Location;          /**< Location of fix */
    fixed Altitude;            /**< GPS Altitude (m) */
    fixed BaroAltitude;        /**< Barometric altitude (m) */
    BrokenDateTime DateTime;    /**< Date and time of fix */
    int SatelliteIDs[MAXSATELLITES]; /**< IDs of satellites in fix */
    fixed Time;                /**< Time of fix (s) */
    int NAVWarning;             /**< GPS fix state */
    int FixQuality;             /**< GPS fix quality */
    int SatellitesUsed;         /**< GPS fix state */
    bool HDOP;                  /**< GPS Horizontal Dilution of precision */
    bool Simulator;             /**< GPS Simulator flag  */

    /** 
     * Set buffer value from NMEA_INFO structure
     * 
     * @param src Item to set
     * 
     * @return Buffer value
     */
    const struct LoggerPreTakeoffBuffer &operator=(const NMEA_INFO &src);
  };
  struct LogPoint_GPSPosition {
    bool Initialized;
    int DegLat;
    int DegLon;
    fixed MinLat;
    fixed MinLon;
    char NoS;
    char EoW;
    int GPSAltitude;
  };

private:
  GRecord oGRecord;
  LogPoint_GPSPosition LastValidPoint;
  BatchBuffer<char[MAX_IGC_BUFF],LOGGER_DISK_BUFFER_NUM_RECS> DiskBuffer;

public:
  /** Default constructor */
  LoggerImpl();

public:
  void LogPoint(const NMEA_INFO &gps_info);
  void LogEvent(const NMEA_INFO &gps_info, const char* event);

  bool isLoggerActive() const;
  static bool LoggerClearFreeSpace(const NMEA_INFO &gps_info);
  void StartLogger(const NMEA_INFO &gps_info,
                   const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber,
                   const Declaration &decl);
  void StopLogger(const NMEA_INFO &gps_info);
  void LoggerNote(const TCHAR *text);
  void clearBuffer();

private:
  void StartLogger(const NMEA_INFO &gps_info,
                   const SETTINGS_COMPUTER &settings,
                   const TCHAR *strAssetNumber);

  void AddDeclaration(const GEOPOINT &location, const TCHAR *ID);
  void StartDeclaration(const NMEA_INFO &gps_info,
                        const int numturnpoints);
  void EndDeclaration(void);
  void LoggerHeader(const NMEA_INFO &gps_info, const Declaration& decl);

  bool IGCWriteRecord(const char *szIn, const TCHAR *);
  void CleanIGCRecord(char * szIn);
  void LoggerGInit();
  
private:
  void LogPointToFile(const NMEA_INFO& gps_info);
  void LogPointToBuffer(const NMEA_INFO &gps_info);
  void LoggerGStop(TCHAR* szLoggerFileName);
  
private:
  void LogFRecordToFile(const int SatelliteIDs[],
                        const BrokenTime broken_time, fixed Time,
                        int NAVWarning);
  void ResetFRecord(void);
  int LastFRecordValid;
  char szLastFRecord[MAX_IGC_BUFF];
  bool DetectFRecordChange;
  GPSClock frecord_clock;
  const char * GetHFFXARecord(void);
  const char * GetIRecord(void);
  fixed GetEPE(const NMEA_INFO& gps_info);
  int GetSIU(const NMEA_INFO& gps_info);
  void LoadGPSPointFromNMEA(const NMEA_INFO& gps_info, LogPoint_GPSPosition &p);

private:
  bool LoggerActive;

  /**
   * If at least one GPS fix came from the simulator
   * (NMEA_INFO.Simulator), the this flag is true, and signing is
   * disabled.
   */
  bool Simulator;

  TCHAR szLoggerFileName[MAX_PATH];
  char szLoggerFileName_c[MAX_PATH];
  OverwritingRingBuffer<LoggerPreTakeoffBuffer,LOGGER_PRETAKEOFF_BUFFER_MAX> PreTakeoffBuffer;

  /* stdio buffering is bad on wince3.0:
   * it appends up to 1024 NULLs at the end of the file if PDA power fails
   * This does not cause SeeYou to crash (today - but it may in the future)
   * NULLs are invalid IGC characters
   * So, we're creating our manual disk buffering system for the IGC files
   */
   
private:
  bool DiskBufferFlush();
  bool DiskBufferAdd(char *sIn);
  void DiskBufferReset();
};

#endif
