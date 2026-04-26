// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Operation.hpp"

#include <fmt/format.h>

void
FlarmDevice::SendPflamVhf(OperationEnvironment &env)
{
  if (!pflam_vhf.IsDefined())
    return;

  char mhz[32];
  pflam_vhf.Format(mhz, sizeof(mhz));
  /* FTD-109: one mandatory VHF channel; optional B/C/D left empty. */
  Send(fmt::format("PFLAM,S,VHF,{0},,,", mhz).c_str(), env);
}

bool
FlarmDevice::PutActiveFrequency(RadioFrequency frequency,
                                [[maybe_unused]] const char *name,
                                OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  pflam_vhf = frequency;
  SendPflamVhf(env);
  return true;
}

bool
FlarmDevice::ExchangeRadioFrequencies(OperationEnvironment &env,
                                     NMEAInfo &info)
{
  auto &s = info.settings;
  if (!s.has_active_frequency.IsValid() || !s.has_standby_frequency.IsValid())
    return true;

  std::swap(s.active_frequency, s.standby_frequency);
  std::swap(s.active_freq_name, s.standby_freq_name);
  s.has_active_frequency.Update(info.clock);
  s.has_standby_frequency.Update(info.clock);
  s.swap_frequencies.Update(info.clock);

  pflam_vhf = s.active_frequency;
  if (!EnableNMEA(env))
    return false;
  SendPflamVhf(env);
  return true;
}
