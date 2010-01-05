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

#if !defined(AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <tchar.h>

struct NMEA_INFO;

class NMEAParser
{
public:
  NMEAParser();
  static void UpdateMonitor(void);
  static bool ParseNMEAString(int portnum,
                              const TCHAR *String, NMEA_INFO *GPS_INFO);
  static void Reset(void);
  static bool PortIsFlarm(int portnum);
  void _Reset(void);

  bool ParseNMEAString_Internal(const TCHAR *String, NMEA_INFO *GPS_INFO);
  bool gpsValid;
  int nSatellites;

  bool activeGPS;
  bool isFlarm;

  static int StartDay;

public:
  static void TestRoutine(NMEA_INFO *GPS_INFO);

  // these routines can be used by other parsers.
  static double ParseAltitude(const TCHAR *, const TCHAR *);
  static size_t ValidateAndExtract(const TCHAR *src, TCHAR *dst, size_t dstsz,
                                   const TCHAR **arr, size_t arrsz);
  static size_t ExtractParameters(const TCHAR *src, TCHAR *dst,
                                  const TCHAR **arr, size_t sz);
  static bool NMEAChecksum(const TCHAR *String);

  static void ExtractParameter(const TCHAR *Source,
			       TCHAR *Destination,
			       int DesiredFieldNumber);

private:
  bool GSAAvailable;
  bool GGAAvailable;
  bool RMZAvailable;
  bool RMAAvailable;
  double RMZAltitude;
  double RMAAltitude;
  double LastTime;

  bool TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO);
  static double TimeModify(double FixTime, NMEA_INFO* info);

  bool GLL(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool GGA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool GSA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMC(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMB(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMZ(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  bool WP0(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool WP1(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool WP2(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  // Additional sentences
  bool PTAS1(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);  // RMN: Tasman instruments.  TAS, Vario, QNE-altitude

  // FLARM sentences
  bool PFLAU(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool PFLAA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
};

extern bool EnableLogNMEA;
void LogNMEA(const TCHAR* text);

#endif
