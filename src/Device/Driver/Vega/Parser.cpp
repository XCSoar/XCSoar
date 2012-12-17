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

static bool
PDSWC(NMEAInputLine &line, NMEAInfo &info, Vega::VolatileData &volatile_data)
{
  unsigned value;
  if (line.ReadChecked(value) &&
      info.settings.ProvideMacCready(fixed(value) / 10, info.clock))
    volatile_data.mc = value;

  auto &switches = info.switch_state;
  auto &vs = switches.vega;
  vs.inputs = line.ReadHex(0);
  vs.outputs = line.ReadHex(0);

  if (vs.GetFlapLanding())
    switches.flap_position = SwitchState::FlapPosition::LANDING;
  else if (vs.GetFlapZero())
    switches.flap_position = SwitchState::FlapPosition::NEUTRAL;
  else if (vs.GetFlapNegative())
    switches.flap_position = SwitchState::FlapPosition::NEGATIVE;
  else if (vs.GetFlapPositive())
    switches.flap_position = SwitchState::FlapPosition::POSITIVE;
  else
    switches.flap_position = SwitchState::FlapPosition::UNKNOWN;

  if (vs.GetUserSwitchMiddle())
    switches.user_switch = SwitchState::UserSwitch::MIDDLE;
  else if (vs.GetUserSwitchUp())
    switches.user_switch = SwitchState::UserSwitch::UP;
  else if (vs.GetUserSwitchDown())
    switches.user_switch = SwitchState::UserSwitch::DOWN;
  else
    switches.user_switch = SwitchState::UserSwitch::UNKNOWN;

  if (vs.GetAirbrakeLocked())
    switches.airbrake_state = SwitchState::AirbrakeState::LOCKED;
  else if (vs.GetAirbrakeNotLocked())
    switches.airbrake_state = SwitchState::AirbrakeState::NOT_LOCKED;
  else
    switches.airbrake_state = SwitchState::AirbrakeState::UNKNOWN;

  switches.flight_mode = vs.GetCircling()
    ? SwitchState::FlightMode::CIRCLING
    : SwitchState::FlightMode::CRUISE;

  if (line.ReadChecked(value)) {
    info.voltage = fixed(value) / 10;
    info.voltage_available.Update(info.clock);
  }

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

  info.stall_ratio = line.Read(fixed(0));
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
    return PDSWC(line, info, volatile_data);
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
