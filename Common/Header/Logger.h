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

#include "XCSoar.h"

extern bool DisableAutoLogger;

void StartLogger(const TCHAR *strAssetNumber);
void LogPoint(double Lattitude, double Longditude, double Altitude,
              double BaroAltitude);
void AddDeclaration(double Lattitude, double Longditude, const TCHAR *ID);
void StartDeclaration(int numturnpoints);
void EndDeclaration(void);
void LoggerHeader(void);
void LoggerNote(const TCHAR *text);
void LoggerDeviceDeclare();

extern bool DeclaredToDevice;
bool CheckDeclaration(void);

bool LoggerClearFreeSpace();
void StopLogger(void);
bool IGCWriteRecord(const char *szIn, const TCHAR *);
void LinkGRecordDLL(void);
bool LoggerGActive();

bool LogFRecordToFile(int SatelliteIDs[], short Hour, short Minute, short Second, bool bAlways);
bool LogFRecord(int SatelliteIDs[], bool bAlways ) ;
void SetFRecordLastTime(double dTime);
double GetFRecordLastTime(void);
void ResetFRecord(void);


#endif


