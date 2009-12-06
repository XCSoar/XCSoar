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

#include "Device/Driver/Vega.hpp"
#include "Device/Internal.hpp"
#include "Protection.hpp"
#include "Message.h"
#include "Device/Parser.h"
#include "Registry.hpp"
#include "DeviceBlackboard.hpp"
#include "InputEvents.h"
#include "LogFile.hpp"
#include "McReady.h"

#include <tchar.h>
#include <stdlib.h>
#include <math.h>

#ifndef _MSC_VER
#include <algorithm>
using std::max;
#endif

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

class VegaDevice : public AbstractDevice {
private:
  ComPort *port;

public:
  VegaDevice(ComPort *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool PutQNH(const AtmosphericPressure& pres);
  virtual bool PutVoice(const TCHAR *sentence);
  virtual bool Declare(const struct Declaration *declaration);
  virtual void OnSysTicker();
};

static bool
PDSWC(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static long last_switchinputs;
  static long last_switchoutputs;
  double MACCREADY = GlidePolar::GetMacCready();

  unsigned long uswitchinputs, uswitchoutputs;
  _stscanf(String,
	  _T("%lf,%lx,%lx,%lf"),
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

  bool is_circling = false;
  switch (device_blackboard.SettingsComputer().EnableExternalTriggerCruise) {
  case 1:
    is_circling = GPS_INFO->SwitchState.FlapLanding;
    break;
  case 2:
    is_circling = GPS_INFO->SwitchState.SpeedCommand;
    break;
  }
  if (is_circling) {
    triggerClimbEvent.trigger();
  } else {
    triggerClimbEvent.reset();
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

  return true;
}

//#include "Audio/VarioSound.h"

static bool
PDAAV(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)GPS_INFO;

  NMEAParser::ExtractParameter(String,ctemp,0);
//  unsigned short beepfrequency = (unsigned short)_tcstod(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,1);
//  unsigned short soundfrequency = (unsigned short)_tcstod(ctemp, NULL);
  NMEAParser::ExtractParameter(String,ctemp,2);
//  unsigned char soundtype = (unsigned char)_tcstod(ctemp, NULL);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return true;
}

static bool
PDVSC(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR name[80];
  TCHAR responsetype[10];
  (void)GPS_INFO;

  NMEAParser::ExtractParameter(String,responsetype,0);
  NMEAParser::ExtractParameter(String,name,1);

  if (_tcscmp(name, _T("ERROR")) == 0){
    // ignore error responses...
    return true;
  }

  NMEAParser::ExtractParameter(String,ctemp,2);
  long value =  (long)_tcstod(ctemp, NULL);
  DWORD dwvalue;

  if (_tcscmp(name, _T("ToneDeadbandCruiseLow"))==0) {
    value = max(value, -value);
  }
  if (_tcscmp(name, _T("ToneDeadbandCirclingLow"))==0) {
    value = max(value, -value);
  }

  TCHAR regname[100];
  _stprintf(regname, _T("Vega%sUpdated"), name);
  SetToRegistry(regname, 1);
  _stprintf(regname, _T("Vega%s"), name);
  dwvalue = *((DWORD*)&value);
  SetToRegistry(regname, dwvalue);

  return true;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static bool
PDVDV(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  TCHAR ctemp[80];
  double alt;

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->Vario = _tcstod(ctemp, NULL) / 10.0;

  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->IndicatedAirspeed = _tcstod(ctemp, NULL) / 10.0;

  NMEAParser::ExtractParameter(String,ctemp,2);
  GPS_INFO->TrueAirspeed = _tcstod(ctemp, NULL) *
    GPS_INFO->IndicatedAirspeed / 1024.0;

  //hasVega = true;
  GPS_INFO->VarioAvailable = true;
  GPS_INFO->AirspeedAvailable = true;

  if (enable_baro){
    NMEAParser::ExtractParameter(String,ctemp,3);
    alt = _tcstod(ctemp, NULL);
    GPS_INFO->BaroAltitudeAvailable = true;
    GPS_INFO->BaroAltitude = GPS_INFO->pressure.
      AltitudeToQNHAltitude(alt);
    // JMW 20080716 bug
      // was alt;    // ToDo check if QNH correction is needed!
  }

  TriggerVarioUpdate();

  return true;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static bool
PDVDS(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double flap;
  TCHAR ctemp[80];

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->AccelX = _tcstod(ctemp, NULL) / 100.0;
  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->AccelZ = _tcstod(ctemp, NULL) / 100.0;

  int mag = isqrt4((int)((GPS_INFO->AccelX*GPS_INFO->AccelX
			  +GPS_INFO->AccelZ*GPS_INFO->AccelZ)*10000));
  GPS_INFO->Gload = mag/100.0;
  GPS_INFO->AccelerationAvailable = true;

  NMEAParser::ExtractParameter(String,ctemp,2);
  flap = _tcstod(ctemp, NULL);

  NMEAParser::ExtractParameter(String,ctemp,3);
  GPS_INFO->StallRatio = _tcstod(ctemp, NULL) / 100.0;

  NMEAParser::ExtractParameter(String,ctemp,4);
  if (ctemp[0] != '\0') {
    GPS_INFO->NettoVarioAvailable = true;
    GPS_INFO->NettoVario = _tcstod(ctemp, NULL) / 10.0;
  } else {
    GPS_INFO->NettoVarioAvailable = false;
  }

  if (device_blackboard.SettingsComputer().EnableCalibration) {
    DebugStore("%g %g %g %g %g %g #te net\r\n",
	    GPS_INFO->IndicatedAirspeed,
	    GPS_INFO->BaroAltitude,
	    GPS_INFO->Vario,
	    GPS_INFO->NettoVario,
	    GPS_INFO->AccelX,
	    GPS_INFO->AccelZ);
  }
  GPS_INFO->VarioAvailable = true;
  //hasVega = true;

  return true;
}

static bool
PDVVT(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->OutsideAirTemperature = _tcstod(ctemp, NULL) / 10.0 - 273.0;
  GPS_INFO->TemperatureAvailable = true;

  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->RelativeHumidity = _tcstod(ctemp, NULL); // %
  GPS_INFO->HumidityAvailable = true;

  return true;
}

// PDTSM,duration_ms,"free text"
static bool
PDTSM(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  int   duration;
  (void)GPS_INFO;

  duration = (int)_tcstod(String, NULL);

  String = _tcschr(String, ',');
  if (String == NULL)
    return false;
  ++String;

  // todo duration handling
  Message::AddMessage(_T("VEGA:"), String);

  return true;
}

bool
VegaDevice::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO,
                      bool enable_baro)
{
  if(_tcsncmp(_T("$PDSWC"), String, 6)==0)
    return PDSWC(&String[7], GPS_INFO);

  if(_tcsncmp(_T("$PDAAV"), String, 6)==0)
    return PDAAV(&String[7], GPS_INFO);

  if(_tcsncmp(_T("$PDVSC"), String, 6)==0)
    return PDVSC(&String[7], GPS_INFO);

  if(_tcsncmp(_T("$PDVDV"), String, 6)==0)
    return PDVDV(&String[7], GPS_INFO, enable_baro);

  if(_tcsncmp(_T("$PDVDS"), String, 6)==0)
    return PDVDS(&String[7], GPS_INFO);

  if(_tcsncmp(_T("$PDVVT"), String, 6)==0)
    return PDVVT(&String[7], GPS_INFO);

  if(_tcsncmp(_T("$PDVSD"), String, 6)==0)
    {
      TCHAR cptext[80];
      wsprintf(cptext,_T("%s"), &String[7]);
      // TODO code: JMW (from Scott)
      // 	Either use something like
      // 		DoStatusMessage(_T("Vario Message"), cptext);
      // 		(then you can assign time and sound to Vario Message)
      // 	or	Message::AddMessage
      Message::AddMessage(cptext);
      return false;
    }
  if(_tcsncmp(_T("$PDTSM"), String, 6)==0)
    {
      return PDTSM(&String[7], GPS_INFO);
    }

  return false;

}

bool
VegaDevice::Declare(const struct Declaration *decl)
{
  (void) decl;

  // ToDo

  return true;
}

bool
VegaDevice::PutVoice(const TCHAR *Sentence)
{
  PortWriteNMEA(port, Sentence);
  return true;
}

#include "Blackboard.hpp"

static void
_VarioWriteSettings(ComPort *port)
{

    TCHAR mcbuf[100];

    wsprintf(mcbuf, _T("PDVMC,%d,%d,%d,%d,%d"),
	     iround(GlidePolar::GetMacCready()*10),
	     iround(device_blackboard.Calculated().VOpt*10),
	     device_blackboard.Calculated().Circling,
	     iround(device_blackboard.Calculated().TerrainAlt),
	     10132); // JMW 20080716 bug
	     //	     iround(QNH*10));

    PortWriteNMEA(port, mcbuf);
}

bool
VegaDevice::PutQNH(const AtmosphericPressure& pres)
{
  (void)pres;

  _VarioWriteSettings(port);

  return true;
}

void
VegaDevice::OnSysTicker()
{
  if (device_blackboard.Basic().VarioAvailable)
    _VarioWriteSettings(port);
}

static Device *
VegaCreateOnComPort(ComPort *com_port)
{
  return new VegaDevice(com_port);
}

const struct DeviceRegister vgaDevice = {
  _T("Vega"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario, // drfLogger if FLARM connected
  VegaCreateOnComPort,
};
