/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Internal.hpp"
#include "Message.hpp"
#include "Input/InputQueue.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Compiler.h"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <stdio.h>
#include <algorithm>

#ifdef _UNICODE
#include <windows.h>
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

static bool
PDSWC(NMEAInputLine &line, NMEAInfo &info)
{
  static long last_switchinputs;
  static long last_switchoutputs;

  unsigned value;
  if (line.ReadChecked(value))
    info.settings.ProvideMacCready(fixed(value) / 10, info.clock);

  long switchinputs = line.ReadHex(0L);
  long switchoutputs = line.ReadHex(0L);

  if (line.ReadChecked(value)) {
    info.voltage = fixed(value) / 10;
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
    ? SwitchInfo::FlightMode::CIRCLING
    : SwitchInfo::FlightMode::CRUISE;
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
  gcc_unused unsigned short beepfrequency = line.Read(0);
  gcc_unused unsigned short soundfrequency = line.Read(0);
  gcc_unused unsigned char soundtype = line.Read(0);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return true;
}

bool
VegaDevice::PDVSC(NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  char responsetype[10];
  line.Read(responsetype, 10);

  char name[80];
  line.Read(name, 80);

  if (strcmp(name, "ERROR") == 0)
    // ignore error responses...
    return true;

  int value = line.Read(0);

  if (strcmp(name, "ToneDeadbandCruiseLow") == 0)
    value = std::max(value, -value);
  if (strcmp(name, "ToneDeadbandCirclingLow") == 0)
    value = std::max(value, -value);

  settings.Lock();
  settings.Set(name, value);
  settings.Unlock();

  return true;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static bool
PDVDV(NMEAInputLine &line, NMEAInfo &info)
{
  int value;

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(fixed(value) / 10);

  bool ias_available = line.ReadChecked(value);
  int tas_ratio = line.Read(1024);
  if (ias_available) {
    const fixed ias = fixed(value) / 10;
    info.ProvideBothAirspeeds(ias, ias * tas_ratio / 1024);
  }

  //hasVega = true;

  if (line.ReadChecked(value))
    info.ProvidePressureAltitude(fixed(value));

  return true;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static bool
PDVDS(NMEAInputLine &line, NMEAInfo &info)
{
  const int accel_x = line.Read(0), accel_z = line.Read(0);

  fixed mag = SmallHypot(fixed(accel_x), fixed(accel_z));
  info.acceleration.ProvideGLoad(mag / 100, true);

  /*
  double flap = line.Read(0.0);
  */
  line.Skip();

  info.stall_ratio = line.Read(fixed_zero);
  info.stall_ratio_available.Update(info.clock);

  int value;
  if (line.ReadChecked(value))
    info.ProvideNettoVario(fixed(value) / 10);

  //hasVega = true;

  return true;
}

static bool
PDVVT(NMEAInputLine &line, NMEAInfo &info)
{
  int value;
  info.temperature_available = line.ReadChecked(value);
  if (info.temperature_available)
    info.temperature = fixed(value) / 10;

  info.humidity_available = line.ReadChecked(info.humidity);

  return true;
}

// PDTSM,duration_ms,"free text"
static bool
PDTSM(NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  /*
  int duration = (int)strtol(String, NULL, 10);
  */
  line.Skip();

  const auto message = line.Rest();

  StaticString<256> buffer;
  buffer.SetASCII(message.begin(), message.end());

  // todo duration handling
  Message::AddMessage(_T("VEGA:"), buffer);

  return true;
}

bool
VegaDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.Read(type, 16);

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
    const auto message = line.Rest();
    StaticString<256> buffer;
    buffer.SetASCII(message.begin(), message.end());
    Message::AddMessage(buffer);
    return true;
  } else if (strcmp(type, "$PDTSM") == 0)
    return PDTSM(line, info);
  else
    return false;
}
