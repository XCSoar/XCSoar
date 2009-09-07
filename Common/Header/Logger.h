/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#if !defined(AFX_LOGGER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_LOGGER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <tchar.h>
#include "NMEA/Info.h"
#include "SettingsComputer.hpp"

void StartLogger(const NMEA_INFO &gps_info, 
		 const SETTINGS_COMPUTER &settings,
		 const TCHAR *strAssetNumber);
void LogPoint(const NMEA_INFO &gps_info);
void AddDeclaration(double Lattitude, double Longditude, const TCHAR *ID);
void StartDeclaration(const NMEA_INFO &gps_info, 
		 int numturnpoints);
void EndDeclaration(void);
void LoggerHeader(const NMEA_INFO &gps_info);
void LoggerNote(const TCHAR *text);
void LoggerDeviceDeclare();

bool CheckDeclaration(void);

bool LoggerClearFreeSpace(const NMEA_INFO &gps_info);
void StopLogger(const NMEA_INFO &gps_info);
bool IGCWriteRecord(const char *szIn, const TCHAR *);
void LinkGRecordDLL(void);
bool LoggerGActive();

bool
LogFRecordToFile(const int SatelliteIDs[], short Hour, short Minute,
                 short Second, bool bAlways);

bool
LogFRecord(const NMEA_INFO &gps_info, bool bAlways);

void SetFRecordLastTime(double dTime);
double GetFRecordLastTime(void);
void ResetFRecord(void);

void guiStartLogger(const NMEA_INFO& gps_info, 
		    const SETTINGS_COMPUTER& settings,
		    bool noAsk = false);
void guiStopLogger(const NMEA_INFO &gps_info,
		   bool noAsk = false);
void guiToggleLogger(const NMEA_INFO& gps_info, 
		     const SETTINGS_COMPUTER& settings,
		     bool noAsk = false);

#endif


