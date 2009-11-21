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

#include "Device/devEW.h"
#include "Device/Internal.hpp"
#include "Device/device.h"
#include "Device/Port.h"

#include <tchar.h>

#define  USESHORTTPNAME   1       // hack, soulf be configurable

// Additional sentance for EW support

static bool fDeclarationPending = false;
static unsigned long lLastBaudrate = 0;
static int nDeclErrorCode = 0;
static int ewDecelTpIndex = 0;

static bool
EWParseNMEA(struct DeviceDescriptor *d, const TCHAR *String,
            NMEA_INFO *GPS_INFO, bool enable_baro)
{
  (void)d;
  (void)String;
  (void)GPS_INFO;
  // no propriatary sentence

  return false;
}


void appendCheckSum(TCHAR *String){
  int i;
  unsigned char CalcCheckSum = 0;
  TCHAR sTmp[4];

  for(i=1; String[i] != '\0'; i++){
    CalcCheckSum = (unsigned char)(CalcCheckSum ^ (unsigned char)String[i]);
  }

  _stprintf(sTmp, _T("%02X\r\n"), CalcCheckSum);
	_tcscat(String, sTmp);

}

static bool
EWTryConnect(struct DeviceDescriptor *d)
{
  int retries=10;
  while (--retries){

    d->Com->WriteString(_T("##\r\n"));         // send IO Mode command
    if (ExpectString(d->Com, _T("IO Mode.\r")))
      return true;

    ExpectString(d->Com, _T("$$$"));                 // empty imput buffer
  }

  nDeclErrorCode = 1;
  return false;
}

static bool
EWDeclAddWayPoint(struct DeviceDescriptor *d, const WAYPOINT *wp);

static bool
EWDeclare(struct DeviceDescriptor *d, const struct Declaration *decl)
{
  TCHAR sTmp[72];
  TCHAR sPilot[13];
  TCHAR sGliderType[9];
  TCHAR sGliderID[9];

  nDeclErrorCode = 0;
  ewDecelTpIndex = 0;
  fDeclarationPending = true;

  d->Com->StopRxThread();

  lLastBaudrate = d->Com->SetBaudrate(9600L);    // change to IO Mode baudrate

  d->Com->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  if (!EWTryConnect(d)) {
    return false;
  }

  _stprintf(sTmp, _T("#SPI"));                  // send SetPilotInfo
  appendCheckSum(sTmp);
  d->Com->WriteString(sTmp);
  Sleep(50);

  _tcsncpy(sPilot, decl->PilotName, 12);               // copy and strip fields
  sPilot[12] = '\0';
  _tcsncpy(sGliderType, decl->AircraftType, 8);
  sGliderType[8] = '\0';
  _tcsncpy(sGliderID, decl->AircraftRego, 8);
  sGliderID[8] = '\0';

  // build string (field 4-5 are GPS info, no idea what to write)
  _stprintf(sTmp, _T("%-12s%-8s%-8s%-12s%-12s%-6s\r"),
           sPilot,
           sGliderType,
           sGliderID,
           _T(""),                              // GPS Model
           _T(""),                              // GPS Serial No.
           _T("")                               // Flight Date,
                                                  // format unknown,
                                                  // left blank (GPS
                                                  // has a RTC)
  );
  d->Com->WriteString(sTmp);

  if (!ExpectString(d->Com, _T("OK\r"))){
    nDeclErrorCode = 1;
    return false;
  };


  /*
  _stprintf(sTmp, _T("#SUI%02d"), 0);           // send pilot name
  appendCheckSum(sTmp);
  d->Com->WriteString(sTmp);
  Sleep(50);
  d->Com->WriteString(PilotsName);
  d->Com->WriteString(_T("\r"));

  if (!ExpectString(d->Com, _T("OK\r"))){
    nDeclErrorCode = 1;
    return false;
  };

  _stprintf(sTmp, _T("#SUI%02d"), 1);           // send type of aircraft
  appendCheckSum(sTmp);
  d->Com->WriteString(sTmp);
  Sleep(50);
  d->Com->WriteString(Class);
  d->Com->WriteString(_T("\r"));

  if (!ExpectString(d->Com, _T("OK\r"))){
    nDeclErrorCode = 1;
    return false;
  };

  _stprintf(sTmp, _T("#SUI%02d"), 2);           // send aircraft ID
  appendCheckSum(sTmp);
  d->Com->WriteString(sTmp);
  Sleep(50);
  d->Com->WriteString(ID);
  d->Com->WriteString(_T("\r"));

  if (!ExpectString(d->Com, _T("OK\r"))){
    nDeclErrorCode = 1;
    return false;
  };
  */

  for (int i=0; i<6; i++){                        // clear all 6 TP's
    _stprintf(sTmp, _T("#CTP%02d"), i);
    appendCheckSum(sTmp);
    d->Com->WriteString(sTmp);
    if (!ExpectString(d->Com, _T("OK\r"))){
      nDeclErrorCode = 1;
      return false;
    };
  }

  for (int j = 0; j < decl->num_waypoints; j++)
    EWDeclAddWayPoint(d, decl->waypoint[j]);

  d->Com->WriteString(_T("NMEA\r\n"));         // switch to NMEA mode

  d->Com->SetBaudrate(lLastBaudrate);            // restore baudrate

  d->Com->SetRxTimeout(0);                       // clear timeout
  d->Com->StartRxThread();                       // restart RX thread

  fDeclarationPending = false;                    // clear decl pending flag

  return nDeclErrorCode == 0; // return true on success

}

static bool
EWDeclAddWayPoint(struct DeviceDescriptor *d, const WAYPOINT *wp)
{
  TCHAR EWRecord[100];
  TCHAR IDString[12];
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;
  short EoW_Flag, NoS_Flag, EW_Flags;

  if (nDeclErrorCode != 0)                        // check for error
    return false;

  if (ewDecelTpIndex > 6){                        // check for max 6 TP's
    return false;
  }

  _tcsncpy(IDString, wp->Name, 6);                // copy at least 6 chars

  while (_tcslen(IDString) < 6)                   // fill up with spaces
    _tcscat(IDString, _T(" "));

  #if USESHORTTPNAME > 0
    _tcscpy(&IDString[3], _T("   "));           // truncate to short name
  #endif

  // prepare lat
  tmp = wp->Location.Latitude;
  NoS = 'N';
  if (tmp < 0)
    {
      NoS = 'S';
      tmp = -tmp;
    }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;


  // prepare long
  tmp = wp->Location.Longitude;
  EoW = 'E';
  if (tmp < 0)
    {
      EoW = 'W';
      tmp = -tmp;
    }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  //	Calc E/W and N/S flags

  //	Clear flags
  EoW_Flag = 0;                                   // prepare flags
  NoS_Flag = 0;
  EW_Flags = 0;

  if (EoW == 'W')
    {
      EoW_Flag = 0x08;
    }
  else
    {
      EoW_Flag = 0x04;
    }
  if (NoS == 'N')
    {
      NoS_Flag = 0x01;
    }
  else
    {
      NoS_Flag = 0x02;
    }
  //  Do the calculation
  EW_Flags = (short)(EoW_Flag | NoS_Flag);

                                                  // setup command string
  _stprintf(EWRecord, _T("#STP%02X%02X%02X%02X%02X%02X%02X%02X%02X%04X%02X%04X"),
                      ewDecelTpIndex,
                      IDString[0],
                      IDString[1],
                      IDString[2],
                      IDString[3],
                      IDString[4],
                      IDString[5],
                      EW_Flags,
                      DegLat, (int)MinLat/10,
                      DegLon, (int)MinLon/10);

  appendCheckSum(EWRecord);                       // complete package with CS and CRLF

  d->Com->WriteString(EWRecord);                 // put it to the logger

  if (!ExpectString(d->Com, _T("OK\r"))){            // wait for response
    nDeclErrorCode = 1;
    return false;
  }

  ewDecelTpIndex = ewDecelTpIndex + 1;            // increase TP index

  return true;
}

static bool
EWLinkTimeout(struct DeviceDescriptor *d)
{
  (void)d;
  if (!fDeclarationPending)
    d->Com->WriteString(_T("NMEA\r\n"));

  return true;
}

const struct DeviceRegister ewDevice = {
  _T("EW Logger"),
  drfGPS | drfLogger,
  EWParseNMEA,			// ParseNMEA
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
  EWLinkTimeout,		// LinkTimeout
  EWDeclare,			// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL				// OnSysTicker
};
