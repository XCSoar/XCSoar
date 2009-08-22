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

#include "Device/devVega.h"
#include "Device/device.h"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Math/FastMath.h"
#include "Math/Pressure.h"
#include "Parser.h"
#include "Device/Port.h"
#include "Registry.hpp"
#include "Device/devVega.h"
#include "Utils.h"
#include "externs.h"
#include "InputEvents.h"
#include "LogFile.hpp"

#include <tchar.h>
#include <math.h>

#define INPUT_BIT_FLAP_POS                  0 // 1 flap pos
#define INPUT_BIT_FLAP_ZERO                 1 // 1 flap zero
#define INPUT_BIT_FLAP_NEG                  2 // 1 flap neg
#define INPUT_BIT_SC                        3 // 1 circling
#define INPUT_BIT_GEAR_EXTENDED             5 // 1 gear extended
#define INPUT_BIT_AIRBRAKENOTLOCKED         6 // 1 airbrake extended
#define INPUT_BIT_ACK                       8 // 1 ack pressed
#define INPUT_BIT_REP                       9 // 1 rep pressed
//#define INPUT_BIT_STALL                     20  // 1 if detected
#define INPUT_BIT_AIRBRAKELOCKED            21 // 1 airbrake locked
#define INPUT_BIT_USERSWUP                  23 // 1 if up
#define INPUT_BIT_USERSWMIDDLE              24 // 1 if middle
#define INPUT_BIT_USERSWDOWN                25
#define OUTPUT_BIT_CIRCLING                 0  // 1 if circling
#define OUTPUT_BIT_FLAP_LANDING             7  // 1 if positive flap

static BOOL PDSWC(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  static long last_switchinputs;
  static long last_switchoutputs;
  (void)d;
  unsigned long uswitchinputs, uswitchoutputs;
  _stscanf(String,
	  TEXT("%lf,%lx,%lx,%lf"),
	  &MACCREADY,
	  &uswitchinputs,
	  &uswitchoutputs,
	  &GPS_INFO->SupplyBatteryVoltage);

  long switchinputs = uswitchinputs;
  long switchoutputs = uswitchoutputs;

  MACCREADY /= 10;
  GPS_INFO->SupplyBatteryVoltage/= 10;

  GPS_INFO->SwitchState.AirbrakeLocked =
    (switchinputs & (1<<INPUT_BIT_AIRBRAKELOCKED))>0;
  GPS_INFO->SwitchState.FlapPositive =
    (switchinputs & (1<<INPUT_BIT_FLAP_POS))>0;
  GPS_INFO->SwitchState.FlapNeutral =
    (switchinputs & (1<<INPUT_BIT_FLAP_ZERO))>0;
  GPS_INFO->SwitchState.FlapNegative =
    (switchinputs & (1<<INPUT_BIT_FLAP_NEG))>0;
  GPS_INFO->SwitchState.GearExtended =
    (switchinputs & (1<<INPUT_BIT_GEAR_EXTENDED))>0;
  GPS_INFO->SwitchState.Acknowledge =
    (switchinputs & (1<<INPUT_BIT_ACK))>0;
  GPS_INFO->SwitchState.Repeat =
    (switchinputs & (1<<INPUT_BIT_REP))>0;
  GPS_INFO->SwitchState.SpeedCommand =
    (switchinputs & (1<<INPUT_BIT_SC))>0;
  GPS_INFO->SwitchState.UserSwitchUp =
    (switchinputs & (1<<INPUT_BIT_USERSWUP))>0;
  GPS_INFO->SwitchState.UserSwitchMiddle =
    (switchinputs & (1<<INPUT_BIT_USERSWMIDDLE))>0;
  GPS_INFO->SwitchState.UserSwitchDown =
    (switchinputs & (1<<INPUT_BIT_USERSWDOWN))>0;
  /*
  GPS_INFO->SwitchState.Stall =
    (switchinputs & (1<<INPUT_BIT_STALL))>0;
  */
  GPS_INFO->SwitchState.VarioCircling =
    (switchoutputs & (1<<OUTPUT_BIT_CIRCLING))>0;
  GPS_INFO->SwitchState.FlapLanding =
    (switchoutputs & (1<<OUTPUT_BIT_FLAP_LANDING))>0;

  if (EnableExternalTriggerCruise != 0) {
    bool is_circling = false;
    switch (EnableExternalTriggerCruise) {
    case 1:
      is_circling = GPS_INFO->SwitchState.FlapLanding;
      break;
    case 2:
      is_circling = GPS_INFO->SwitchState.SpeedCommand;
      break;
    }
    if (is_circling) {
      ExternalTriggerCruise = false;
      ExternalTriggerCircling = true;
    } else {
      ExternalTriggerCruise = true;
      ExternalTriggerCircling = false;
    }
  } else {
    ExternalTriggerCruise = false;
  }

  long up_switchinputs;
  long down_switchinputs;
  long up_switchoutputs;
  long down_switchoutputs;

  // detect changes to ON: on now (x) and not on before (!lastx)
  // detect changes to OFF: off now (!x) and on before (lastx)

  down_switchinputs = (switchinputs & (~last_switchinputs));
  up_switchinputs = ((~switchinputs) & (last_switchinputs));
  down_switchoutputs = (switchoutputs & (~last_switchoutputs));
  up_switchoutputs = ((~switchoutputs) & (last_switchoutputs));

  int i;
  long thebit;
  for (i=0; i<32; i++) {
    thebit = 1<<i;
    if ((down_switchinputs & thebit) == thebit) {
      InputEvents::processNmea(i);
    }
    if ((down_switchoutputs & thebit) == thebit) {
      InputEvents::processNmea(i+32);
    }
    if ((up_switchinputs & thebit) == thebit) {
      InputEvents::processNmea(i+64);
    }
    if ((up_switchoutputs & thebit) == thebit) {
      InputEvents::processNmea(i+96);
    }
  }

  last_switchinputs = switchinputs;
  last_switchoutputs = switchoutputs;

  return TRUE;
}

#include "VarioSound.h"

static BOOL PDAAV(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)GPS_INFO;
  (void)d;
  NMEAParser::ExtractParameter(String,ctemp,0);
//  unsigned short beepfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,1);
//  unsigned short soundfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,2);
//  unsigned char soundtype = (unsigned char)StrToDouble(ctemp, NULL);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return TRUE;
}

static BOOL PDVSC(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR name[80];
  TCHAR responsetype[10];
  (void)GPS_INFO;
  (void)d;
  NMEAParser::ExtractParameter(String,responsetype,0);
  NMEAParser::ExtractParameter(String,name,1);

  if (_tcscmp(name, TEXT("ERROR")) == 0){
    // ignore error responses...
    return TRUE;
  }

  NMEAParser::ExtractParameter(String,ctemp,2);
  long value =  (long)StrToDouble(ctemp,NULL);
  DWORD dwvalue;

  if (_tcscmp(name, TEXT("ToneDeadbandCruiseLow"))==0) {
    value = max(value, -value);
  }
  if (_tcscmp(name, TEXT("ToneDeadbandCirclingLow"))==0) {
    value = max(value, -value);
  }

  TCHAR regname[100];
  _stprintf(regname, TEXT("Vega%sUpdated"), name);
  SetToRegistry(regname, 1);
  _stprintf(regname, TEXT("Vega%s"), name);
  dwvalue = *((DWORD*)&value);
  SetToRegistry(regname, dwvalue);

  return TRUE;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static BOOL PDVDV(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  double alt;

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->Vario = StrToDouble(ctemp,NULL)/10.0;

  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->IndicatedAirspeed = StrToDouble(ctemp,NULL)/10.0;

  NMEAParser::ExtractParameter(String,ctemp,2);
  GPS_INFO->TrueAirspeed = StrToDouble(ctemp,NULL)*GPS_INFO->IndicatedAirspeed/1024.0;

  //hasVega = true;
  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->AirspeedAvailable = TRUE;

  if (d == pDevPrimaryBaroSource){
    NMEAParser::ExtractParameter(String,ctemp,3);
    alt = StrToDouble(ctemp,NULL);
    GPS_INFO->BaroAltitudeAvailable = TRUE;
    GPS_INFO->BaroAltitude = // JMW 20080716 bug
      AltitudeToQNHAltitude(alt);
      // was alt;    // ToDo check if QNH correction is needed!
  }

  TriggerVarioUpdate();

  return TRUE;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static BOOL PDVDS(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  double flap;
  TCHAR ctemp[80];
  (void)d;

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->AccelX = StrToDouble(ctemp,NULL)/AccelerometerZero;
  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->AccelZ = StrToDouble(ctemp,NULL)/AccelerometerZero;

  int mag = isqrt4((int)((GPS_INFO->AccelX*GPS_INFO->AccelX
			  +GPS_INFO->AccelZ*GPS_INFO->AccelZ)*10000));
  GPS_INFO->Gload = mag/100.0;
  GPS_INFO->AccelerationAvailable = TRUE;

  NMEAParser::ExtractParameter(String,ctemp,2);
  flap = StrToDouble(ctemp,NULL);

  NMEAParser::ExtractParameter(String,ctemp,3);
  GPS_INFO->StallRatio = StrToDouble(ctemp,NULL)/100.0;

  NMEAParser::ExtractParameter(String,ctemp,4);
  if (ctemp[0] != '\0') {
    GPS_INFO->NettoVarioAvailable = TRUE;
    GPS_INFO->NettoVario = StrToDouble(ctemp,NULL)/10.0;
  } else {
    GPS_INFO->NettoVarioAvailable = FALSE;
  }

  if (EnableCalibration) {
    DebugStore("%g %g %g %g %g %g #te net\r\n",
	    GPS_INFO->IndicatedAirspeed,
	    GPS_INFO->BaroAltitude,
	    GPS_INFO->Vario,
	    GPS_INFO->NettoVario,
	    GPS_INFO->AccelX,
	    GPS_INFO->AccelZ);
  }
  GPS_INFO->VarioAvailable = TRUE;
  //hasVega = true;

  return TRUE;
}

static BOOL PDVVT(PDeviceDescriptor_t d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)d;
  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->OutsideAirTemperature = StrToDouble(ctemp,NULL)/10.0-273.0;
  GPS_INFO->TemperatureAvailable = TRUE;

  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->RelativeHumidity = StrToDouble(ctemp,NULL); // %
  GPS_INFO->HumidityAvailable = TRUE;

  return TRUE;
}

// PDTSM,duration_ms,"free text"
static BOOL PDTSM(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  int   duration;
  (void)GPS_INFO;
  (void)d;

  duration = (int)StrToDouble(String, NULL);

  String = _tcschr(String, ',');
  if (String == NULL)
    return FALSE;
  ++String;

  // todo duration handling
  DoStatusMessage(TEXT("VEGA:"), String);

  return TRUE;

}



BOOL vgaParseNMEA(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  if(_tcsncmp(TEXT("$PDSWC"), String, 6)==0)
    {
      return PDSWC(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDAAV"), String, 6)==0)
    {
      return PDAAV(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDVSC"), String, 6)==0)
    {
      return PDVSC(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDVDV"), String, 6)==0)
    {
      return PDVDV(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDVDS"), String, 6)==0)
    {
      return PDVDS(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDVVT"), String, 6)==0)
    {
      return PDVVT(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(TEXT("$PDVSD"), String, 6)==0)
    {
      TCHAR cptext[80];
      wsprintf(cptext,TEXT("%s"), &String[7]);
      // TODO code: JMW (from Scott)
      // 	Either use something like
      // 		DoStatusMessage(TEXT("Vario Message"), cptext);
      // 		(then you can assign time and sound to Vario Message)
      // 	or	Message::AddMessage
      DoStatusMessage(cptext);
      return FALSE;
    }
  if(_tcsncmp(TEXT("$PDTSM"), String, 6)==0)
    {
      return PDTSM(d, &String[7], GPS_INFO);
    }

  return FALSE;

}


BOOL vgaDeclare(PDeviceDescriptor_t d, Declaration_t *decl){
  (void) d;
  (void) decl;

  // ToDo

  return(TRUE);

}


BOOL vgaPutVoice(PDeviceDescriptor_t d, const TCHAR *Sentence)
{
  devWriteNMEAString(d, Sentence);
  return(TRUE);
}

static void _VarioWriteSettings(DeviceDescriptor_t *d) {

    TCHAR mcbuf[100];

    wsprintf(mcbuf, TEXT("PDVMC,%d,%d,%d,%d,%d"),
	     iround(MACCREADY*10),
	     iround(CALCULATED_INFO.VOpt*10),
	     CALCULATED_INFO.Circling,
	     iround(CALCULATED_INFO.TerrainAlt),
	     10132); // JMW 20080716 bug
	     //	     iround(QNH*10));

    devWriteNMEAString(d, mcbuf);
}


BOOL vgaPutQNH(DeviceDescriptor_t *d, double NewQNH){
  (void)NewQNH;
  // NewQNH is already stored in QNH

  _VarioWriteSettings(d);

  return(TRUE);
}

BOOL vgaOnSysTicker(DeviceDescriptor_t *d){

  if (GPS_INFO.VarioAvailable)
    _VarioWriteSettings(d);

  return(TRUE);
}


static const DeviceRegister_t vgaDevice = {
  TEXT("Vega"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario, // drfLogger if FLARM connected
  vgaParseNMEA,			// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  vgaPutQNH,			// PutQNH
  vgaPutVoice,			// PutVoice
  NULL,				// PutVolume
  NULL,				// PutFreqActive
  NULL,				// PutFreqStandby
  NULL,				// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  vgaDeclare,			// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource: only if GPS source connected to Vega.NmeaIn
  NULL,				// IsBaroSource
  vgaOnSysTicker		// OnSysTicker
};

bool vgaRegister(void){
  return devRegister(&vgaDevice);
}

