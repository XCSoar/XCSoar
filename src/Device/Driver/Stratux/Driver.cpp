// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Driver.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
#include "Device/Driver/FLARM/StaticParser.hpp"

StratuxDevice::StratuxSettings settings;

void
LoadFromProfile(StratuxDevice::StratuxSettings &settings) noexcept
{
  bool ok=false;

  ok |= Profile::Get(ProfileKeys::StratuxHorizontalRange,settings.hrange);
  ok |= Profile::Get(ProfileKeys::StratuxVerticalRange,settings.vrange);

  if (!ok) {
    settings.hrange = 20000;
    settings.vrange = 2000;
  }
}

void
SaveToProfile(StratuxDevice::StratuxSettings &settings) noexcept
{
    Profile::Set(ProfileKeys::StratuxHorizontalRange, settings.hrange);
    Profile::Set(ProfileKeys::StratuxVerticalRange, settings.vrange);
}

using std::string_view_literals::operator""sv;

bool
StratuxDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  NMEAInputLine input_line(line);

  const auto type = input_line.ReadView();
  if (type == "$PFLAA"sv) {
    RangeFilter range;
    range.horizontal = settings.hrange;
    range.vertical = settings.vrange;
    ParsePFLAA(input_line, info.flarm.traffic, info.clock, range);
    return true;
  } else
    return false;
}

static Device *
StratuxCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  LoadFromProfile(settings);

  return new StratuxDevice();
}

const struct DeviceRegister stratux_driver = {
  _T("Stratux"),
  _T("Stratux"),
  DeviceRegister::MANAGE,
  StratuxCreateOnPort,
};
