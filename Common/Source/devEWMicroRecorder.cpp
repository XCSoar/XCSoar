/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#include "StdAfx.h"


#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devEWMicroRecorder.h"


// Additional sentance for EW support

static int nDeclErrorCode = 0;
#define MAX_USER_SIZE 2500
static int user_size = 0;
static TCHAR user_data[MAX_USER_SIZE];


BOOL ExpectStringWait(PDeviceDescriptor_t d, TCHAR *token) {

  int i=0, ch;
  int j=0;

  if (!d->Com)
    return FALSE;

  while (j<500) {

    ch = d->Com->GetChar();

    if (ch != EOF) {

      if (token[i] == ch)
        i++;
      else
        i=0;

      if ((unsigned)i == _tcslen(token))
        return(TRUE);
    }
    j++;
  }

  #if debugIGNORERESPONCE > 0
  return(TRUE);
  #endif
  return(FALSE);

}



BOOL EWMicroRecorderParseNMEA(PDeviceDescriptor_t d,
                              TCHAR *String, NMEA_INFO *GPS_INFO){
  TCHAR ctemp[80], *params[5];
  int nparams = NMEAParser::ValidateAndExtract(String, ctemp, 80, params, 5);
  if (nparams < 1)
    return FALSE;

  if (!_tcscmp(params[0], TEXT("$PGRMZ")) && nparams >= 3) {
    if (d == pDevPrimaryBaroSource) {
      double altitude = NMEAParser::ParseAltitude(params[1], params[2]);

      GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(altitude);
      GPS_INFO->BaroAltitudeAvailable = true;
    }

    return TRUE;
  }

  return FALSE;
}


BOOL EWMicroRecorderTryConnect(PDeviceDescriptor_t d) {
  int retries=10;
  TCHAR ch;

  while (--retries){

    d->Com->WriteString(TEXT("\x02"));         // send IO Mode command

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
          d->Com->WriteString(TEXT("\x16"));
          user_data[user_size] = 0;
          // found end of file
          return TRUE;
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
  return(FALSE);
}


static void EWMicroRecorderPrintf(PDeviceDescriptor_t d, const TCHAR *fmt, ...)
{
  TCHAR EWStr[128];
  va_list ap;

  va_start(ap, fmt);
  _vstprintf(EWStr, fmt, ap);
  va_end(ap);

  d->Com->WriteString(EWStr);
}

static void EWMicroRecorderWriteWayPoint(PDeviceDescriptor_t d,
                                         const WAYPOINT *wp,
                                         TCHAR* EWType) {
  TCHAR EWRecord[128];
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  TCHAR NoS, EoW;

  // prepare latitude
  tmp = wp->Latitude;
  NoS = _T('N');
  if (tmp < 0)
    {
      NoS = _T('S');
      tmp = -tmp;
    }

  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = wp->Longitude;
  EoW = _T('E');
  if (tmp < 0)
    {
      EoW = _T('W');
      tmp = -tmp;
    }

  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  EWMicroRecorderPrintf(d,
            TEXT("%-17s %02d%05d%c%03d%05d%c %s\r\n"),
            EWType,
            DegLat, (int)MinLat, NoS,
            DegLon, (int)MinLon, EoW,
            wp->Name);
}


BOOL EWMicroRecorderDeclare(PDeviceDescriptor_t d, Declaration_t *decl)
{
  const WAYPOINT *wp;
  nDeclErrorCode = 0;

  // Must have at least two, max 12 waypoints
  if (decl->num_waypoints < 2 || decl->num_waypoints > 12)
    return FALSE;

  d->Com->StopRxThread();

  d->Com->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  if (!EWMicroRecorderTryConnect(d)) {
    return FALSE;
  }

  d->Com->WriteString(TEXT("\x18"));         // start to upload file
  d->Com->WriteString(user_data);
  EWMicroRecorderPrintf(d, TEXT("%-15s %s\r\n"),
               TEXT("Pilot Name:"), decl->PilotName);
  EWMicroRecorderPrintf(d, TEXT("%-15s %s\r\n"),
               TEXT("Competition ID:"), decl->AircraftRego);
  EWMicroRecorderPrintf(d, TEXT("%-15s %s\r\n"),
               TEXT("Aircraft Type:"), decl->AircraftType);
  d->Com->WriteString(TEXT("Description:      Declaration\r\n"));

  for (int i = 0; i < 11; i++) {
    wp = decl->waypoint[i];
    if (i == 0) {
      EWMicroRecorderWriteWayPoint(d, wp, TEXT("Take Off LatLong:"));
      EWMicroRecorderWriteWayPoint(d, wp, TEXT("Start LatLon:"));
    } else if (i + 1 < decl->num_waypoints) {
      EWMicroRecorderWriteWayPoint(d, wp, TEXT("TP LatLon:"));
    } else {
      EWMicroRecorderPrintf(d, TEXT("%-17s %s\r\n"),
               TEXT("TP LatLon:"), TEXT("0000000N00000000E TURN POINT\r\n"));
    }
  }

  wp = decl->waypoint[decl->num_waypoints - 1];
  EWMicroRecorderWriteWayPoint(d, wp, TEXT("Finish LatLon:"));
  EWMicroRecorderWriteWayPoint(d, wp, TEXT("Land LatLon:"));

  d->Com->WriteString(TEXT("\x03"));         // finish sending user file

  if (!ExpectStringWait(d, TEXT("uploaded successfully"))) {
    // error!
    nDeclErrorCode = 1;
  }
  d->Com->WriteString(TEXT("!!\r\n"));         // go back to NMEA mode

  d->Com->SetRxTimeout(0);                       // clear timeout
  d->Com->StartRxThread();                       // restart RX thread

  return(nDeclErrorCode == 0);                    // return() TRUE on success
}



BOOL EWMicroRecorderIsTrue(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL ewMicroRecorderInstall(PDeviceDescriptor_t d){
  d->ParseNMEA = EWMicroRecorderParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->LinkTimeout = NULL;
  d->Declare = EWMicroRecorderDeclare;
  d->IsLogger = EWMicroRecorderIsTrue;
  d->IsGPSSource = EWMicroRecorderIsTrue;
  d->IsBaroSource = EWMicroRecorderIsTrue;

  return(TRUE);

}


static const DeviceRegister_t ewMicroRecorderDevice = {
  TEXT("EW MicroRecorder"),
  drfGPS | drfLogger | drfBaroAlt,
  ewMicroRecorderInstall
};

BOOL ewMicroRecorderRegister(void){
  return devRegister(&ewMicroRecorderDevice);
}



