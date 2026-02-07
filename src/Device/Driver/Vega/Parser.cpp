// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Message.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <tchar.h>
#include <algorithm>

using std::string_view_literals::operator""sv;

static bool
PDSWC(NMEAInputLine &line, NMEAInfo &info, Vega::VolatileData &volatile_data)
{
  unsigned value;
  if (line.ReadChecked(value) &&
      info.settings.ProvideMacCready(value / 10., info.clock))
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
    info.voltage = value / 10.;
    info.voltage_available.Update(info.clock);
  }

  return true;
}

//#include "Audio/VarioSound.h"

static bool
PDAAV(NMEAInputLine &line, [[maybe_unused]] NMEAInfo &info)
{
  [[maybe_unused]] unsigned short beepfrequency = line.Read(0);
  [[maybe_unused]] unsigned short soundfrequency = line.Read(0);
  [[maybe_unused]] unsigned char soundtype = line.Read(0);

  // Temporarily commented out - function as yet undefined
  //  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return true;
}

bool
VegaDevice::PDVSC(NMEAInputLine &line, [[maybe_unused]] NMEAInfo &info)
{
  [[maybe_unused]] const auto responsetype = line.ReadView();

  const auto name = line.ReadView();

  if (name == "ERROR"sv)
    // ignore error responses...
    return true;

  int value = line.Read(0);

  if (name == "ToneDeadbandCruiseLow"sv)
    value = std::max(value, -value);
  else if (name == "ToneDeadbandCirclingLow"sv)
    value = std::max(value, -value);

  {
    const std::lock_guard<Mutex> lock(settings);
    settings.Set(std::string{name}, value);
  }

  return true;
}


// $PDVDV,vario,ias,densityratio,altitude,staticpressure

static bool
PDVDV(NMEAInputLine &line, NMEAInfo &info)
{
  int value;

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value / 10.);

  bool ias_available = line.ReadChecked(value);
  int tas_ratio = line.Read(1024);
  if (ias_available) {
    const auto ias = value / 10.;
    info.ProvideBothAirspeeds(ias, ias * tas_ratio / 1024);
  }

  //hasVega = true;

  if (line.ReadChecked(value))
    info.ProvidePressureAltitude(value);

  return true;
}


// $PDVDS,nx,nz,flap,stallratio,netto
static bool
PDVDS(NMEAInputLine &line, NMEAInfo &info)
{
  const int accel_x = line.Read(0), accel_z = line.Read(0);

  auto mag = hypot(accel_x, accel_z);
  info.acceleration.ProvideGLoad(mag / 100);

  /*
  double flap = line.Read(0.0);
  */
  line.Skip();

  info.stall_ratio = line.Read(0.);
  info.stall_ratio_available.Update(info.clock);

  int value;
  if (line.ReadChecked(value))
    info.ProvideNettoVario(value / 10.);

  //hasVega = true;

  return true;
}

static bool
PDVVT(NMEAInputLine &line, NMEAInfo &info)
{
  int value;
  info.temperature_available = line.ReadChecked(value);
  if (info.temperature_available)
    info.temperature = Temperature::FromKelvin(value / 10.);

  info.humidity_available = line.ReadChecked(info.humidity);

  return true;
}

// PDTSM,duration_ms,"free text"
static bool
PDTSM(NMEAInputLine &line, [[maybe_unused]] NMEAInfo &info)
{
  /*
  int duration = (int)strtol(String, nullptr, 10);
  */
  line.Skip();

  const auto message = line.Rest();

  StaticString<256> buffer;
  buffer.SetASCII(message);

  // todo duration handling
  Message::AddMessage("VEGA:", buffer);

  return true;
}

bool
VegaDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);

  const auto type = line.ReadView();

  if (type.starts_with("$PD"sv))
    detected = true;

  if (type == "$PDSWC"sv)
    return PDSWC(line, info, volatile_data);
  else if (type == "$PDAAV"sv)
    return PDAAV(line, info);
  else if (type == "$PDVSC"sv)
    return PDVSC(line, info);
  else if (type == "$PDVDV"sv)
    return PDVDV(line, info);
  else if (type == "$PDVDS"sv)
    return PDVDS(line, info);
  else if (type == "$PDVVT"sv)
    return PDVVT(line, info);
  else if (type == "$PDVSD"sv) {
    const auto message = line.Rest();
    StaticString<256> buffer;
    buffer.SetASCII(message);
    Message::AddMessage(buffer);
    return true;
  } else if (type == "$PDTSM"sv)
    return PDTSM(line, info);
  else
    return false;
}
