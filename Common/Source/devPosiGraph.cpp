/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

// created 20070428 sgi, basic NMEA sentance supported
// WARNING PosiGraph dont send any GGA sentance
// GGA sentance trigger nav process in XCSoar
// Posigraph driver trigger Nav process with RMC sentance (if driver is devA())

// ToDo

// adding baro alt sentance paser to support baro source priority  if (d == pDevPrimaryBaroSource){...}




#include "stdafx.h"


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devPosiGraph.h"


#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif

static BOOL GPWIN(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO);

BOOL PGParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){
  (void)d;
  (void)String;
  (void)GPS_INFO;

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if(_tcsncmp(TEXT("$GPWIN"), String, 6)==0)
    {
      return GPWIN(d, &String[7], GPS_INFO);
    }

  return FALSE;

}


BOOL PGOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  return(TRUE);
}

BOOL PGDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  (void)d;
  (void)PilotsName;
  (void)Class;
  (void)ID;

  return(FALSE);

}


BOOL PGDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  (void)d;
  (void)wp;

  return(FALSE);

}

BOOL PGDeclEnd(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL PGIsLogger(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL PGIsGPSSource(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}

BOOL PGIsBaroSource(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}

BOOL PGLinkTimeout(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL pgInstall(PDeviceDescriptor_t d){

  _tcscpy(d->Name, TEXT("PosiGraph Logger"));
  d->ParseNMEA = PGParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = PGOpen;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = PGLinkTimeout;
  d->DeclBegin = PGDeclBegin;
  d->DeclEnd = PGDeclEnd;
  d->DeclAddWayPoint = PGDeclAddWayPoint;
  d->IsLogger = PGIsLogger;
  d->IsGPSSource = PGIsGPSSource;
  d->IsBaroSource = PGIsBaroSource;

  return(TRUE);

}


BOOL pgRegister(void){
  return(devRegister(
    TEXT("PosiGraph Logger"),
      1l << dfGPS
      | (1l << dfBaroAlt)
      | 1l << dfLogger,
    pgInstall
  ));
}


// *****************************************************************************
// local stuff

static BOOL GPWIN(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)GPS_INFO;
  (void)d;

  NMEAParser::ExtractParameter(String, ctemp, 2);

  if (d == pDevPrimaryBaroSource){
    GPS_INFO->BaroAltitudeAvailable = TRUE;
    GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(iround(StrToDouble(ctemp, NULL)/10));
  }

  return FALSE;

}
