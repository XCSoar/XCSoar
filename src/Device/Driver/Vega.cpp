/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Device/Driver.hpp"
#include "Message.hpp"
#include "Profile/Profile.hpp"
#include "DeviceBlackboard.hpp"
#include "InputEvents.hpp"
#include "LogFile.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

using std::max;

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
  Port *port;

  /**
   * The most recent QNH value, written by SetQNH(), read by
   * VarioWriteSettings().
   */
  AtmosphericPressure qnh;

  bool detected;

public:
  VegaDevice(Port *_port)
    :port(_port), detected(false) {}

protected:
  void VarioWriteSettings(const DerivedInfo &calculated) const;

public:
  virtual void LinkTimeout();
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
  virtual bool PutQNH(const AtmosphericPressure& pres);
  virtual bool PutVoice(const TCHAR *sentence);
  virtual void OnSysTicker(const DerivedInfo &calculated);
};

void
VegaDevice::LinkTimeout()
{
  AbstractDevice::LinkTimeout();
  detected = false;
}

static bool
PDSWC(NMEAInputLine &line, NMEAInfo &info)
{
  static long last_switchinputs;
  static long last_switchoutputs;

  fixed value;
  if (line.read_checked(value))
    info.settings.ProvideMacCready(value / 10, info.clock);

  long switchinputs = line.read_hex(0L);
  long switchoutputs = line.read_hex(0L);

  if (line.read_checked(value)) {
    info.voltage = value / 10;
    info.voltage_available.Update(info.clock);
  }

  info.switch_state_available = true;

  info.switch_state.airbrake_locked =
    (switchinputs & (1<<INPUT_BIT_AIRBRAKELOCKED))>0;
  info.switch_state.flap_positive =
    (switchinputs & (1<<INPUT_BIT_FLAP_POS))>0;
  info.switch_state.flap_neutral =
    (switchinputs & (1<<INPUT_BIT_FLAP_ZERO))>0;
  info.switch_state.flap_negative =
    (switchinputs & (1<<INPUT_BIT_FLAP_NEG))>0;
  info.switch_state.gear_extended =
    (switchinputs & (1<<INPUT_BIT_GEAR_EXTENDED))>0;
  info.switch_state.acknowledge =
    (switchinputs & (1<<INPUT_BIT_ACK))>0;
  info.switch_state.repeat =
    (switchinputs & (1<<INPUT_BIT_REP))>0;
  info.switch_state.speed_command =
    (switchinputs & (1<<INPUT_BIT_SC))>0;
  info.switch_state.user_switch_up =
    (switchinputs & (1<<INPUT_BIT_USERSWUP))>0;
  info.switch_state.user_switch_middle =
    (switchinputs & (1<<INPUT_BIT_USERSWMIDDLE))>0;
  info.switch_state.user_switch_down =
    (switchinputs & (1<<INPUT_BIT_USERSWDOWN))>0;
  /*
  info.switch_state.Stall =
    (switchinputs & (1<<INPUT_BIT_STALL))>0;
  */
  info.switch_state.flight_mode =
    (switchoutputs & (1 << OUTPUT_BIT_CIRCLING)) > 0
    ? SwitchInfo::MODE_CIRCLING
    : SwitchInfo::MODE_CRUISE;
  info.switch_state.flap_landing =
    (switchoutputs & (1<<OUTPUT_BIT_FLAP_LANDING))>0;

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
PDAAV(NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  gcc_unused unsigned short beepfrequency = line.read(0);
  gcc_unused unsigned short soundfrequency = line.read(0);
  gcc_unused unsigned char soundtype = line.read(0);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return true;
}

static bool
PDVSC(NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  char responsetype[10];
  line.read(responsetype, 10);

  char name[80];
  line.read(name, 80);

  if (strcmp(name, "ERROR") == 0)
    // ignore error responses...
    return true;

  long value = line.read(0L);

  if (strcmp(name, "ToneDeadbandCruiseLow") == 0)
    value = max(value, -value);
  if (strcmp(name, "ToneDeadbandCirclingLow") == 0)
    value = max(value, -value);

  TCHAR regname[100];

  _stprintf(regname, _T("Vega%sUpdated"), name);
  Profile::Set(regname, 1);

  _stprintf(regname, _T("Vega%s"), name);
  Profile::Set(regname, value);

  return true;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static bool
PDVDV(NMEAInputLine &line, NMEAInfo &info)
{
  fixed value;

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value / 10);

  bool ias_available = line.read_checked(value);
  fixed tas_ratio = line.read(fixed(1024)) / 1024;
  if (ias_available)
    info.ProvideBothAirspeeds(value / 10, value / 10 * tas_ratio);

  //hasVega = true;

  if (line.read_checked(value))
    info.ProvidePressureAltitude(value);

  return true;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static bool
PDVDS(NMEAInputLine &line, NMEAInfo &info)
{
  fixed AccelX = line.read(fixed_zero);
  fixed AccelZ = line.read(fixed_zero);

  int mag = (int)hypot(AccelX, AccelZ);
  info.acceleration.g_load = fixed(mag) / 100;
  info.acceleration.available = true;

  /*
  double flap = line.read(0.0);
  */
  line.skip();

  info.stall_ratio = line.read(fixed_zero);
  info.stall_ratio_available.Update(info.clock);

  fixed value;
  if (line.read_checked(value))
    info.ProvideNettoVario(value / 10);

  //hasVega = true;

  return true;
}

static bool
PDVVT(NMEAInputLine &line, NMEAInfo &info)
{
  fixed value;
  info.temperature_available = line.read_checked(value);
  if (info.temperature_available)
    info.temperature = Units::ToSysUnit(value / 10, unGradCelcius);

  info.humidity_available = line.read_checked(info.humidity);

  return true;
}

// PDTSM,duration_ms,"free text"
static bool
PDTSM(NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  /*
  int duration = (int)strtol(String, NULL, 10);
  */
  line.skip();

  const char *message = line.rest();
#ifdef _UNICODE
  TCHAR buffer[strlen(message)];
  if (MultiByteToWideChar(CP_ACP, 0, message, -1,
                          buffer, sizeof(buffer) / sizeof(buffer[0])) <= 0)
    return false;
#else
  const char *buffer = message;
#endif

  // todo duration handling
  Message::AddMessage(_T("VEGA:"), buffer);

  return true;
}

bool
VegaDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (memcmp(type, "$PD", 3) == 0)
    detected = true;

  if (strcmp(type, "$PDSWC") == 0)
    return PDSWC(line, info);
  else if (strcmp(type, "$PDAAV") == 0)
    return PDAAV(line, info);
  else if (strcmp(type, "$PDVSC") == 0)
    return PDVSC(line, info);
  else if (strcmp(type, "$PDVDV") == 0)
    return PDVDV(line, info);
  else if (strcmp(type, "$PDVDS") == 0)
    return PDVDS(line, info);
  else if (strcmp(type, "$PDVVT") == 0)
    return PDVVT(line, info);
  else if (strcmp(type, "$PDVSD") == 0) {
    const char *message = line.rest();
#ifdef _UNICODE
    TCHAR buffer[strlen(message)];
    if (MultiByteToWideChar(CP_ACP, 0, message, -1,
                            buffer, sizeof(buffer) / sizeof(buffer[0])) <= 0)
      return false;
#else
    const char *buffer = message;
#endif

    Message::AddMessage(buffer);
    return true;
  } else if (strcmp(type, "$PDTSM") == 0)
    return PDTSM(line, info);
  else
    return false;
}

bool
VegaDevice::PutVoice(const TCHAR *Sentence)
{
#ifdef _UNICODE
  char buffer[_tcslen(Sentence) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, Sentence, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return false;
#else
  const char *buffer = Sentence;
#endif

  PortWriteNMEA(port, buffer);
  return true;
}

void
VegaDevice::VarioWriteSettings(const DerivedInfo &calculated) const
{
    char mcbuf[100];

    sprintf(mcbuf, "PDVMC,%d,%d,%d,%d,%d",
            iround(calculated.common_stats.current_mc*10),
            iround(calculated.V_stf*10),
            calculated.circling,
            iround(calculated.terrain_altitude),
            uround(qnh.get_QNH() * 10));

    PortWriteNMEA(port, mcbuf);
}

bool
VegaDevice::PutQNH(const AtmosphericPressure& pres)
{
  qnh = pres;

  return true;
}

void
VegaDevice::OnSysTicker(const DerivedInfo &calculated)
{
  if (detected)
    VarioWriteSettings(calculated);

#ifdef UAV_APPLICATION
  const ThermalLocatorInfo &t = calculated.thermal_locator;
  char tbuf[100];
  sprintf(tbuf, "PTLOC,%d,%3.5f,%3.5f,%g,%g",
          (int)(t.estimate_valid),
          (double)t.estimate_location.Longitude.value_degrees(),
          (double)t.estimate_location.Latitude.value_degrees(),
          (double)fixed_zero,
          (double)fixed_zero);

  PortWriteNMEA(port, tbuf);
#endif
}

static Device *
VegaCreateOnPort(const DeviceConfig &config, Port *com_port)
{
  return new VegaDevice(com_port);
}

const struct DeviceRegister vgaDevice = {
  _T("Vega"),
  _T("Vega"),
  DeviceRegister::MANAGE,
  VegaCreateOnPort,
};
