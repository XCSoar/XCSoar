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

#include "StdAfx.h"

#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devFlymasterF1.h"

static BOOL VARIO(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO);

static BOOL FlymasterF1ParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){
  (void)d;

  if(_tcsncmp(TEXT("$VARIO"), String, 6)==0)
    {
      return VARIO(d, &String[7], GPS_INFO);
    }

  return FALSE;

}


static BOOL FlymasterF1IsLogger(PDeviceDescriptor_t d){
  (void)d;
  return(FALSE);
}


static BOOL FlymasterF1IsGPSSource(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


static BOOL FlymasterF1IsBaroSource(PDeviceDescriptor_t d){
	(void)d;
  return(TRUE);
}


static BOOL FlymasterF1LinkTimeout(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


static BOOL flymasterf1Install(PDeviceDescriptor_t d){

  _tcscpy(d->Name, TEXT("FlymasterF1"));
  d->ParseNMEA = FlymasterF1ParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = FlymasterF1LinkTimeout;
  d->Declare = NULL;
  d->IsGPSSource = FlymasterF1IsGPSSource;
  d->IsBaroSource = FlymasterF1IsBaroSource;

  return(TRUE);

}


BOOL flymasterf1Register(void){
  return(devRegister(
    TEXT("FlymasterF1"),
    (1l << dfGPS)
    | (1l << dfBaroAlt)
    | (1l << dfVario),
    flymasterf1Install
  ));
}


// *****************************************************************************
// local stuff

static BOOL VARIO(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  TCHAR ctemp[80];
  NMEAParser::ExtractParameter(String,ctemp,0);
  double ps = StrToDouble(ctemp,NULL);
  GPS_INFO->BaroAltitude = (1 - pow(fabs(ps / QNH),  0.190284)) * 44307.69;
  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->Vario = StrToDouble(ctemp,NULL)/10.0;
  // JMW vario is in dm/s

  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->BaroAltitudeAvailable = TRUE;

  TriggerVarioUpdate();

  return TRUE;
}
