// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "LX1600.hpp"
#include "LXNAVVario.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"

#include <fmt/format.h>

bool
LXDevice::SendLXNAVVarioSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(lxnav_vario_settings);
    lxnav_vario_settings.MarkOld(name);
  }

  const auto buffer = fmt::format("PLXV0,{},W,{}", name, value);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

bool
LXDevice::RequestLXNAVVarioSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(lxnav_vario_settings);
    lxnav_vario_settings.MarkOld(name);
  }

  const auto buffer = fmt::format("PLXV0,{},R", name);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

std::string
LXDevice::WaitLXNAVVarioSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  std::unique_lock<Mutex> lock(lxnav_vario_settings);
  auto i = lxnav_vario_settings.Wait(lock, name, env,
                            std::chrono::milliseconds(timeout_ms));
  if (i == lxnav_vario_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetLXNAVVarioSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(lxnav_vario_settings);
  auto i = lxnav_vario_settings.find(name);
  if (i == lxnav_vario_settings.end())
    return std::string();

  return *i;
}

bool
LXDevice::SendNanoSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableLoggerNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(nano_settings);
    nano_settings.MarkOld(name);
  }

  const auto buffer = fmt::format("PLXVC,SET,W,{},{}", name, value);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

bool
LXDevice::SendNanoSetting(const char *name, unsigned value,
                          OperationEnvironment &env)
{
  const auto str = fmt::format("{}", value);
  return SendNanoSetting(name, str.c_str(), env);
}

bool
LXDevice::RequestNanoSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableLoggerNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(nano_settings);
    nano_settings.MarkOld(name);
  }

  const auto buffer = fmt::format("PLXVC,SET,R,{}", name);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

std::string
LXDevice::WaitNanoSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  std::unique_lock<Mutex> lock(nano_settings);
  auto i = nano_settings.Wait(lock, name, env,
                              std::chrono::milliseconds(timeout_ms));
  if (i == nano_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetNanoSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(nano_settings);
  auto i = nano_settings.find(name);
  if (i == nano_settings.end())
    return std::string();

  return *i;
}

unsigned
LXDevice::GetNanoSettingInteger(const char *name) const noexcept
{
  const std::lock_guard<Mutex> lock{nano_settings};
  auto i = nano_settings.find(name);
  if (i == nano_settings.end())
    return {};

  return strtoul(i->c_str(), nullptr, 10);
}

bool
LXDevice::PutBallast([[maybe_unused]] double fraction, double overload,
                     OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetBallast(port, env, overload);
  else
    LX1600::SetBallast(port, env, overload);
  
  /* Track what we sent for feedback loop detection */
  {
    const std::lock_guard lock{mutex};
    last_sent_ballast_overload = overload;
  }
  
  return true;
}

bool
LXDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  int transformed_bugs_value = 100 - (int)(bugs*100);

  if (IsLXNAVVario())
    LXNAVVario::SetBugs(port, env, transformed_bugs_value);
  else
    LX1600::SetBugs(port, env, transformed_bugs_value);
  
  /* Track what we sent for feedback loop detection */
  {
    const std::lock_guard lock{mutex};
    last_sent_bugs = bugs;
  }
  
  return true;
}

bool
LXDevice::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetMacCready(port, env, mac_cready);
  else
    LX1600::SetMacCready(port, env, mac_cready);
  
  /* Track what we sent for feedback loop detection */
  {
    const std::lock_guard lock{mutex};
    last_sent_mc = mac_cready;
  }
  
  return true;
}

bool
LXDevice::PutCrewMass(double crew_mass, OperationEnvironment &env)
{
  /* Only support crew mass sync for LXNAV varios */
  if (!IsLXNAVVario())
    return true;

  if (!EnableNMEA(env))
    return false;

  /* Send only pilot weight, leaving all other POLAR fields empty */
  LXNAVVario::SetPilotWeight(port, env, crew_mass);
  
  /* Track what we sent */
  {
    const std::lock_guard lock{mutex};
    last_sent_crew_mass = crew_mass;
  }
  
  return true;
}

bool
LXDevice::PutEmptyMass(double empty_mass, OperationEnvironment &env)
{
  /* Only support empty mass sync for LXNAV varios */
  if (!IsLXNAVVario())
    return true;

  if (!EnableNMEA(env))
    return false;

  /* Send only empty weight, leaving all other POLAR fields empty */
  LXNAVVario::SetEmptyWeight(port, env, empty_mass);
  
  /* Track what we sent */
  {
    const std::lock_guard lock{mutex};
    last_sent_empty_mass = empty_mass;
  }
  
  return true;
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetQNH(port, env, pres);
  else
    LX1600::SetQNH(port, env, pres);
  return true;
}

bool
LXDevice::PutElevation(int elevation, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetElevation(port, env, elevation);
  else
    return false;

  return true;
}

bool
LXDevice::RequestElevation(OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    return RequestLXNAVVarioSetting("ELEVATION", env);

  return false;
}

bool
LXDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetVolume(port, env, volume);
  else if (IsLX16xx())
    LX1600::SetVolume(port, env, volume);
  else
    return false;

  return true;
}

bool
LXDevice::PutPilotEvent(OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return false;

  LXNAVVario::PutPilotEvent(env, port);
  return true;
}

bool
LXDevice::PutActiveFrequency(RadioFrequency frequency,
                             const char *name,
                             OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return false;

  if (!EnableNMEA(env))
    return false;

  /* LXNAV uses frequency in kHz without decimal point:
     e.g. 128.800 MHz → 128800 */
  const unsigned freq_khz = frequency.GetKiloHertz();
  const auto buffer = (name != nullptr && name[0] != '\0')
    ? fmt::format("PLXVC,RADIO,S,COMM,{},{}", freq_khz, name)
    : fmt::format("PLXVC,RADIO,S,COMM,{}", freq_khz);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

bool
LXDevice::PutStandbyFrequency(RadioFrequency frequency,
                              const char *name,
                              OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return false;

  if (!EnableNMEA(env))
    return false;

  const unsigned freq_khz = frequency.GetKiloHertz();
  const auto buffer = (name != nullptr && name[0] != '\0')
    ? fmt::format("PLXVC,RADIO,S,SBY,{},{}", freq_khz, name)
    : fmt::format("PLXVC,RADIO,S,SBY,{}", freq_khz);
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}

bool
LXDevice::ExchangeRadioFrequencies(OperationEnvironment &env,
                                   NMEAInfo &info)
{
  if (!IsLXNAVVario())
    return false;

  /* Swap active and standby in NMEAInfo */
  if (info.settings.has_active_frequency.IsValid() &&
      info.settings.has_standby_frequency.IsValid()) {
    std::swap(info.settings.active_frequency,
              info.settings.standby_frequency);
    std::swap(info.settings.active_freq_name,
              info.settings.standby_freq_name);
    info.settings.swap_frequencies.Update(info.clock);

    /* Send both frequencies to the device */
    PutActiveFrequency(info.settings.active_frequency,
                       info.settings.active_freq_name,
                       env);
    PutStandbyFrequency(info.settings.standby_frequency,
                        info.settings.standby_freq_name,
                        env);
  }
  return true;
}

bool
LXDevice::PutTransponderCode(TransponderCode code,
                             OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return false;

  if (!EnableNMEA(env))
    return false;

  /* LXNAV uses display squawk format (e.g. "7700") which is octal */
  const auto buffer =
    fmt::format("PLXVC,XPDR,S,SQUAWK,{:04o}", code.GetCode());
  PortWriteNMEA(port, buffer.c_str(), env);
  return true;
}
