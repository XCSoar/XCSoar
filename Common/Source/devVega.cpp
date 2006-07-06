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

#include "devVega.h"


#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif


#include "InputEvents.h"


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

static BOOL PDSWC(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static long last_switchinputs;
  static long last_switchoutputs;

  unsigned long uswitchinputs, uswitchoutputs;
  swscanf(String,
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

  if (EnableExternalTriggerCruise) {
    if (!GPS_INFO->SwitchState.FlapPositive) {
      // JMW TODO: Change to OUTPUT_BIT_FLAP_LANDING
      ExternalTriggerCruise = true;
    } else {
      ExternalTriggerCruise = false;
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

static BOOL PDAAV(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  NMEAParser::ExtractParameter(String,ctemp,0);
  unsigned short beepfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,1);
  unsigned short soundfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,2);
  unsigned char soundtype = (unsigned char)StrToDouble(ctemp, NULL);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);
  
  return FALSE;
}

static BOOL PDVSC(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR name[80];
  TCHAR responsetype[10];
  NMEAParser::ExtractParameter(String,responsetype,0);
  NMEAParser::ExtractParameter(String,name,1);
  NMEAParser::ExtractParameter(String,ctemp,2);
  long value =  (long)StrToDouble(ctemp,NULL);
  DWORD dwvalue;

  TCHAR updatename[100];
  TCHAR fullname[100];
  _stprintf(updatename, TEXT("Vega%sUpdated"), name);
  _stprintf(fullname, TEXT("Vega%s"), name);
  SetToRegistry(updatename, 1);
  dwvalue = *((DWORD*)&value);
  SetToRegistry(fullname, dwvalue);

  /*
  wsprintf(ctemp,TEXT("%s"), &String[0]);
  DoStatusMessage(ctemp);
  */
  return FALSE;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static BOOL PDVDV(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  
  double alt;
  
  swscanf(String,
	  TEXT("%lf,%lf,%lf,%lf"),
	  &GPS_INFO->Vario, //
	  &GPS_INFO->IndicatedAirspeed,
	  &GPS_INFO->TrueAirspeed,
	  &alt);

  GPS_INFO->Vario /= 10.0;
  GPS_INFO->VarioAvailable = TRUE;
  //hasVega = true;

  GPS_INFO->IndicatedAirspeed /= 10.0;
  GPS_INFO->AirspeedAvailable = TRUE;
  GPS_INFO->TrueAirspeed *= GPS_INFO->IndicatedAirspeed/1024.0;

  if (d == pDevPrimaryBaroSource){
    GPS_INFO->BaroAltitudeAvailable = TRUE;
    GPS_INFO->BaroAltitude = alt;
  }

  NMEAParser::VarioUpdated = TRUE;
  PulseEvent(varioTriggerEvent);

  return FALSE;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static BOOL PDVDS(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double flap, stallratio;

  int found = swscanf(String,
	  TEXT("%lf,%lf,%lf,%lf,%lf"),
	  &GPS_INFO->AccelX,
	  &GPS_INFO->AccelZ,
	  &flap,
	  &stallratio,
	  &GPS_INFO->NettoVario);

  GPS_INFO->AccelX /= AccelerometerZero;
  GPS_INFO->AccelZ /= AccelerometerZero;
  int mag = isqrt4((int)((GPS_INFO->AccelX*GPS_INFO->AccelX
			  +GPS_INFO->AccelZ*GPS_INFO->AccelZ)*10000));
  GPS_INFO->Gload = mag/100.0;

//#pragma message( "----------------->>>> Experimantal remove later! <<<<----------------------") 
//GPS_INFO->Gload = stallratio/100.0;

  GPS_INFO->AccelerationAvailable = TRUE;
  if (found==5) {
	  GPS_INFO->NettoVarioAvailable = TRUE; 
  } else {
	  GPS_INFO->NettoVarioAvailable = FALSE; 
  }
  GPS_INFO->NettoVario /= 10.0;

  if (EnableCalibration) {
    char buffer[200];
    sprintf(buffer,"%g %g %g %g %g %g #te net\r\n",
	    GPS_INFO->IndicatedAirspeed,
	    GPS_INFO->BaroAltitude,
	    GPS_INFO->Vario, 
	    GPS_INFO->NettoVario, 
	    GPS_INFO->AccelX, 
	    GPS_INFO->AccelZ); 
    DebugStore(buffer);
  }
  GPS_INFO->VarioAvailable = TRUE;
  //hasVega = true;

  return FALSE;
}

static BOOL PDVVT(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->OutsideAirTemperature = StrToDouble(ctemp,NULL)/10.0-273.0;
  GPS_INFO->TemperatureAvailable = TRUE;

  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->RelativeHumidity = StrToDouble(ctemp,NULL); // %
  GPS_INFO->HumidityAvailable = TRUE;

  return FALSE;
}

// PDTSM,duration_ms,"free text"
static BOOL PDTSM(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){

  int   duration;
  TCHAR  *pWClast = NULL;
  TCHAR  *pToken;

  if ((pToken = strtok_r(String, TEXT(","), &pWClast)) == NULL)
    return FALSE;

  duration = (int)StrToDouble(pToken, NULL);

  if ((pToken = strtok_r(NULL, TEXT("*"), &pWClast)) == NULL)
    return FALSE;

  // todo duration handling
  DoStatusMessage(TEXT("VEGA:"), pToken);

  return FALSE;

}



BOOL vgaParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){



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
	    // TODO - JMW (from Scott)
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


BOOL vgaOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  return(TRUE);
}


BOOL vgaDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  (void) d;
  (void) PilotsName;
  (void) ID;

  // ToDo

  return(TRUE);

}


BOOL vgaDeclEnd(PDeviceDescriptor_t d){

  (void) d;

  // ToDo

  return(TRUE);                    // return() TRUE on success

}


BOOL vgaDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  (void) d;
  (void) wp;

  // ToDo

  return(TRUE);

}


BOOL vgaIsLogger(PDeviceDescriptor_t d){
//  return(TRUE);
  return(FALSE);
}

BOOL vgaIsGPSSource(PDeviceDescriptor_t d){
  return(TRUE);  // this is only true if GPS source is connected on VEGA.NmeaIn
}

BOOL vgaIsBaroSource(PDeviceDescriptor_t d){
  return(TRUE);
}

BOOL vgaPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence){
  (d->Com.WriteNMEAString)(Sentence);
  return(TRUE);
}

static void _VarioWriteSettings(DeviceDescriptor_t *d) {

    TCHAR mcbuf[100];

    wsprintf(mcbuf, TEXT("PDVMC,%d,%d,%d,%d,%d"),
	     iround(MACCREADY*10),
	     iround(CALCULATED_INFO.VOpt*10),
	     CALCULATED_INFO.Circling,
	     iround(CALCULATED_INFO.TerrainAlt),
	     iround(QNH*10));

    
    (d->Com.WriteNMEAString)(mcbuf);


}


BOOL vgaPutQNH(DeviceDescriptor_t *d, double NewQNH){

  // NewQNH is already stored in QNH

  _VarioWriteSettings(d);

  return(TRUE);
}

BOOL vgaOnSysTicker(DeviceDescriptor_t *d){

  _VarioWriteSettings(d);
  
  return(TRUE);
}


BOOL vgaInstall(PDeviceDescriptor_t d){

  _tcscpy(d->Name, TEXT("Vega"));
  d->ParseNMEA = vgaParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = vgaOpen;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->DeclBegin = vgaDeclBegin;
  d->DeclEnd = vgaDeclEnd;
  d->DeclAddWayPoint = vgaDeclAddWayPoint;
  d->IsLogger = vgaIsLogger;
  d->IsGPSSource = vgaIsGPSSource;
  d->IsBaroSource = vgaIsBaroSource;
  d->PutVoice = vgaPutVoice;
  d->PutQNH = vgaPutQNH;
  d->OnSysTicker = vgaOnSysTicker;

  return(TRUE);

}


BOOL vgaRegister(void){
  return(devRegister(
    TEXT("Vega"), 
      (1l << dfGPS)
    | (1l << dfBaroAlt)
    | (1l << dfSpeed)
    | (1l << dfVario)

//      | 1l << dfLogger     // if FLARM connected
    ,
    vgaInstall
  ));
}

