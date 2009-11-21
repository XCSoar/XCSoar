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


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/devEWMicroRecorder.h"
#include "Device/Internal.hpp"
#include "Device/device.h"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "NMEA/Info.h"

#include <tchar.h>
#include <stdio.h>

#define debugIGNORERESPONCE 0

// Additional sentance for EW support

static int nDeclErrorCode = 0;
#define MAX_USER_SIZE 2500
static int user_size = 0;
static TCHAR user_data[MAX_USER_SIZE];

static bool
ExpectStringWait(struct DeviceDescriptor *d, const TCHAR *token)
{
  int i=0, ch;
  int j=0;

  if (!d->Com)
    return false;

  while (j<500) {

    ch = d->Com->GetChar();

    if (ch != EOF) {

      if (token[i] == ch)
        i++;
      else
        i=0;

      if ((unsigned)i == _tcslen(token))
        return true;
    }
    j++;
  }

  #if debugIGNORERESPONCE > 0
  return true;
  #endif
  return false;
}

static bool
EWMicroRecorderParseNMEA(struct DeviceDescriptor *d, const TCHAR *String,
                         NMEA_INFO *GPS_INFO, bool enable_baro)
{
  TCHAR ctemp[80];
  const TCHAR *params[5];
  int nparams = NMEAParser::ValidateAndExtract(String, ctemp, 80, params, 5);
  if (nparams < 1)
    return false;

  if (!_tcscmp(params[0], _T("$PGRMZ")) && nparams >= 3) {
    if (enable_baro) {
      double altitude = NMEAParser::ParseAltitude(params[1], params[2]);

      GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(altitude);
      GPS_INFO->BaroAltitudeAvailable = true;
    }

    return true;
  }

  return false;
}

static bool
EWMicroRecorderTryConnect(struct DeviceDescriptor *d)
{
  int retries=10;
  TCHAR ch;

  while (--retries){

    d->Com->WriteString(_T("\x02"));         // send IO Mode command

    user_size = 0;
    bool started = false;

    while ((ch = d->Com->GetChar()) != _TEOF) {
      if (!started) {
        if (ch == _T('-')) {
          started = true;
        }
      }
      if (started) {
        if (ch == 0x13) {
          d->Com->WriteString(_T("\x16"));
          user_data[user_size] = 0;
          // found end of file
          return true;
        } else {
          if (user_size<MAX_USER_SIZE-1) {
            user_data[user_size] = ch;
            user_size++;
          }
        }
      }
    }

  }

  nDeclErrorCode = 1;
  return false;
}


static void
EWMicroRecorderPrintf(struct DeviceDescriptor *d, const TCHAR *fmt, ...)
{
  TCHAR EWStr[128];
  va_list ap;

  va_start(ap, fmt);
  _vstprintf(EWStr, fmt, ap);
  va_end(ap);

  d->Com->WriteString(EWStr);
}

static void
EWMicroRecorderWriteWayPoint(struct DeviceDescriptor *d,
                             const WAYPOINT *wp, const TCHAR *EWType)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  TCHAR NoS, EoW;

  // prepare latitude
  tmp = wp->Location.Latitude;
  NoS = _T('N');
  if (tmp < 0)
    {
      NoS = _T('S');
      tmp = -tmp;
    }

  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = wp->Location.Longitude;
  EoW = _T('E');
  if (tmp < 0)
    {
      EoW = _T('W');
      tmp = -tmp;
    }

  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  EWMicroRecorderPrintf(d,
            _T("%-17s %02d%05d%c%03d%05d%c %s\r\n"),
            EWType,
            DegLat, (int)MinLat, NoS,
            DegLon, (int)MinLon, EoW,
            wp->Name);
}

static bool
EWMicroRecorderDeclare(struct DeviceDescriptor *d,
                       const struct Declaration *decl)
{
  const WAYPOINT *wp;
  nDeclErrorCode = 0;

  // Must have at least two, max 12 waypoints
  if (decl->num_waypoints < 2 || decl->num_waypoints > 12)
    return false;

  d->Com->StopRxThread();

  d->Com->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  if (!EWMicroRecorderTryConnect(d)) {
    return false;
  }

  d->Com->WriteString(_T("\x18"));         // start to upload file
  d->Com->WriteString(user_data);
  EWMicroRecorderPrintf(d, _T("%-15s %s\r\n"),
               _T("Pilot Name:"), decl->PilotName);
  EWMicroRecorderPrintf(d, _T("%-15s %s\r\n"),
               _T("Competition ID:"), decl->AircraftRego);
  EWMicroRecorderPrintf(d, _T("%-15s %s\r\n"),
               _T("Aircraft Type:"), decl->AircraftType);
  d->Com->WriteString(_T("Description:      Declaration\r\n"));

  for (int i = 0; i < 11; i++) {
    wp = decl->waypoint[i];
    if (i == 0) {
      EWMicroRecorderWriteWayPoint(d, wp, _T("Take Off LatLong:"));
      EWMicroRecorderWriteWayPoint(d, wp, _T("Start LatLon:"));
    } else if (i + 1 < decl->num_waypoints) {
      EWMicroRecorderWriteWayPoint(d, wp, _T("TP LatLon:"));
    } else {
      EWMicroRecorderPrintf(d, _T("%-17s %s\r\n"),
               _T("TP LatLon:"), _T("0000000N00000000E TURN POINT\r\n"));
    }
  }

  wp = decl->waypoint[decl->num_waypoints - 1];
  EWMicroRecorderWriteWayPoint(d, wp, _T("Finish LatLon:"));
  EWMicroRecorderWriteWayPoint(d, wp, _T("Land LatLon:"));

  d->Com->WriteString(_T("\x03"));         // finish sending user file

  if (!ExpectStringWait(d, _T("uploaded successfully"))) {
    // error!
    nDeclErrorCode = 1;
  }
  d->Com->WriteString(_T("!!\r\n"));         // go back to NMEA mode

  d->Com->SetRxTimeout(0);                       // clear timeout
  d->Com->StartRxThread();                       // restart RX thread

  return nDeclErrorCode == 0; // return true on success
}

const struct DeviceRegister ewMicroRecorderDevice = {
  _T("EW MicroRecorder"),
  drfGPS | drfLogger | drfBaroAlt,
  EWMicroRecorderParseNMEA,	// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  NULL,				// PutQNH
  NULL,				// PutVoice
  NULL,				// PutVolume
  NULL,				// PutFreqActive
  NULL,				// PutFreqStandby
  NULL,				// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  EWMicroRecorderDeclare,	// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL				// OnSysTicker
};
