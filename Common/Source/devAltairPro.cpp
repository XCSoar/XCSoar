/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2006  

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


  Created: 2006.04.07 / samuel gisiger triadis engineering GmbH 

}
*/

#include "stdafx.h"


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devAltairPro.h"

double AltitudeToQNHAltitude(double alt);

#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif

static double lastAlt = 0;


BOOL atrParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){

  // no propriatary sentence

  if (_tcsncmp(TEXT("$PGRMZ"), String, 6) == 0){

    TCHAR  *pWClast = NULL;
    TCHAR  *pToken;

    // eat $PGRMZ
    if ((pToken = strtok_r(String, TEXT(","), &pWClast)) == NULL)
      return FALSE;

    // get <alt>
    if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
      return FALSE;

    lastAlt = StrToDouble(pToken, NULL);; 
    
    // get <unit>
    if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
      return FALSE;

    if (*pToken == 'f' || *pToken== 'F')
      lastAlt /= TOFEET;

    
    if (d == pDevPrimaryBaroSource){
      GPS_INFO->BaroAltitudeAvailable = TRUE;
      GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(lastAlt);
    }

    return(TRUE);
  }
  
  return FALSE;

}


BOOL atrOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  return(TRUE);
}


BOOL atrDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  (void) d;
  (void) PilotsName;
  (void) ID;

  // ToDo

  return(TRUE);

}


BOOL atrDeclEnd(PDeviceDescriptor_t d){

  (void) d;

  // ToDo

  return(TRUE);                    // return() TRUE on success

}


BOOL atrDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  (void) d;
  (void) wp;

  // ToDo

  return(TRUE);

}


BOOL atrIsLogger(PDeviceDescriptor_t d){
//  return(TRUE);
  return(FALSE);
}


BOOL atrIsGPSSource(PDeviceDescriptor_t d){
  return(TRUE);
}

BOOL atrIsBaroSource(PDeviceDescriptor_t d){
  return(TRUE);
}

BOOL atrPutQNH(DeviceDescriptor_t *d, double NewQNH){

  if (d == pDevPrimaryBaroSource){
    GPS_INFO.BaroAltitude = AltitudeToQNHAltitude(lastAlt);
  }

  return(TRUE);
}

BOOL atrOnSysTicker(DeviceDescriptor_t *d){

  // Do To get IO data like temp, humid, etc
  
  return(TRUE);
}

BOOL atrInstall(PDeviceDescriptor_t d){
  _tcscpy(d->Name, TEXT("Altair Pro"));
  d->ParseNMEA = atrParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = atrOpen;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->DeclBegin = atrDeclBegin;
  d->DeclEnd = atrDeclEnd;
  d->DeclAddWayPoint = atrDeclAddWayPoint;
  d->IsLogger = atrIsLogger;
  d->IsGPSSource = atrIsGPSSource;
  d->IsBaroSource = atrIsBaroSource;
  d->PutQNH = atrPutQNH;
  d->OnSysTicker = atrOnSysTicker;

  return(TRUE);

}


BOOL atrRegister(void){
  return(devRegister(
    TEXT("Altair Pro"), 
      (1l << dfGPS)
    | (1l << dfBaroAlt)
//      | 1l << dfLogger  // ToDo
    ,
    atrInstall
  ));
}

